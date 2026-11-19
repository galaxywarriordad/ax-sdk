#include "ax/topology.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

#ifdef __linux__
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <avrt.h>
#pragma comment(lib, "avrt.lib")
#endif

namespace ax::topology {
namespace {

struct PhysicalCore {
    std::uint32_t primary_logical_cpu = 0;
    std::uint32_t efficiency_score = 1024; // Higher = P-core / higher capacity
    bool has_smt_sibling = false;
};

void BuildProportionalMasks(const std::vector<PhysicalCore>& cores,
                            SiliconProfile& profile) noexcept {
    if (cores.empty()) {
        return; // Retain safe 6-core defaults (0x01, 0x02, 0x1C, 0x20)
    }

    const std::size_t n = cores.size();
    auto bit_for = [&](std::size_t idx) noexcept -> std::uint64_t {
        const std::uint32_t cpu = cores[idx].primary_logical_cpu;
        return (cpu < 64U) ? (1ULL << cpu) : 0ULL;
    };

    if (n < 4) {
        // Fallback for <4 core systems: avoid hard lockups while preserving domain separation
        profile.audio_mask = bit_for(0);
        profile.vulkan_mask = (n >= 2) ? bit_for(1) : bit_for(0);
        profile.jit_mask = 0;
        for (std::size_t i = 0; i < n; ++i) {
            profile.jit_mask |= bit_for(i);
        }
        profile.housekeeping_mask = bit_for(n - 1);
        return;
    }

    // 2:3:1 proportional allocation across primary physical P-cores:
    //   1 dedicated core  -> AudioTiming (Core 0)
    //   1 dedicated core  -> VulkanSubmission (Core 1)
    //   ~50% of cores     -> GuestJit (min 2 cores, 3 on 6-core i5-9600K)
    //   remaining cores   -> Housekeeping / Async VRAM Reaper / SPIR-V / HID
    const std::size_t jit_count = std::clamp<std::size_t>(n / 2, 2, n - 3);
    const std::size_t jit_end = 2 + jit_count;

    profile.audio_mask = bit_for(0);
    profile.vulkan_mask = bit_for(1);

    std::uint64_t jit = 0;
    for (std::size_t i = 2; i < jit_end && i < n; ++i) {
        jit |= bit_for(i);
    }
    profile.jit_mask = (jit != 0) ? jit : bit_for(1);

    std::uint64_t hk = 0;
    for (std::size_t i = jit_end; i < n; ++i) {
        hk |= bit_for(i);
    }
    profile.housekeeping_mask = (hk != 0) ? hk : bit_for(n - 1);
}

#ifdef _WIN32

std::vector<PhysicalCore> EnumerateWin32PhysicalCores(bool& out_smt_detected) noexcept {
    std::vector<PhysicalCore> cores;
    out_smt_detected = false;

    DWORD buffer_bytes = 0;
    ::GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &buffer_bytes);
    if (buffer_bytes == 0 || ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return cores;
    }

    std::vector<std::uint8_t> buffer(buffer_bytes);
    auto* ptr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());
    if (!::GetLogicalProcessorInformationEx(RelationProcessorCore, ptr, &buffer_bytes)) {
        return cores;
    }

    DWORD offset = 0;
    while (offset < buffer_bytes) {
        const auto* info = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
            buffer.data() + offset);
        if (info->Relationship == RelationProcessorCore && info->Processor.GroupCount > 0) {
            // Restrict to Processor Group 0 (standard <=64 logical CPU desktop topology)
            const GROUP_AFFINITY& group0 = info->Processor.GroupMask[0];
            if (group0.Group == 0 && group0.Mask != 0) {
                DWORD first_cpu = 0;
                if (_BitScanForward64(&first_cpu, static_cast<unsigned __int64>(group0.Mask))) {
                    const bool is_smt = (info->Processor.Flags & LTP_PC_SMT) != 0;
                    if (is_smt) {
                        out_smt_detected = true;
                    }
                    // Windows EfficiencyClass: higher values indicate P-cores on Intel hybrid CPUs
                    const std::uint32_t eff_score =
                        1024U + static_cast<std::uint32_t>(info->Processor.EfficiencyClass) * 256U;
                    cores.push_back(PhysicalCore{
                        .primary_logical_cpu = static_cast<std::uint32_t>(first_cpu),
                        .efficiency_score = eff_score,
                        .has_smt_sibling = is_smt,
                    });
                }
            }
        }
        offset += info->Size;
    }

    // Sort P-cores (higher efficiency_score) first, then ascending logical CPU index
    std::stable_sort(cores.begin(), cores.end(), [](const PhysicalCore& a, const PhysicalCore& b) {
        if (a.efficiency_score != b.efficiency_score) {
            return a.efficiency_score > b.efficiency_score;
        }
        return a.primary_logical_cpu < b.primary_logical_cpu;
    });

    return cores;
}

class MmcssRegistration {
public:
    MmcssRegistration() = default;
    ~MmcssRegistration() {
        Revert();
    }
    MmcssRegistration(const MmcssRegistration&) = delete;
    MmcssRegistration& operator=(const MmcssRegistration&) = delete;

    void EnableProAudio() noexcept {
        if (handle_ != nullptr) {
            return;
        }
        DWORD task_index = 0;
        handle_ = ::AvSetMmThreadCharacteristicsW(L"Pro Audio", &task_index);
        if (handle_ != nullptr) {
            ::AvSetMmThreadPriority(handle_, AVRT_PRIORITY_CRITICAL);
        }
    }

    void Revert() noexcept {
        if (handle_ != nullptr) {
            ::AvRevertMmThreadCharacteristics(handle_);
            handle_ = nullptr;
        }
    }

private:
    HANDLE handle_ = nullptr;
};

#elif defined(__linux__)

bool ReadSysfsUint(const char* path, std::uint32_t& out_val) noexcept {
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    char buf[32]{};
    const ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
    ::close(fd);
    if (n <= 0) {
        return false;
    }
    std::uint32_t parsed = 0;
    const auto [ptr, ec] = std::from_chars(buf, buf + n, parsed);
    if (ec != std::errc{}) {
        return false;
    }
    out_val = parsed;
    return true;
}

bool ParsePrimarySibling(std::uint32_t cpu, std::uint32_t& out_primary,
                         bool& out_has_sibling) noexcept {
    char path[128]{};
    std::snprintf(path, sizeof(path),
                  "/sys/devices/system/cpu/cpu%u/topology/thread_siblings_list", cpu);
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    char buf[128]{};
    const ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
    ::close(fd);
    if (n <= 0) {
        return false;
    }

    std::uint32_t first_id = cpu;
    const auto [ptr, ec] = std::from_chars(buf, buf + n, first_id);
    if (ec != std::errc{}) {
        return false;
    }

    out_primary = first_id;
    // Presence of ',' or '-' in thread_siblings_list indicates >=2 SMT threads on this core
    const std::string_view sv(buf, static_cast<std::size_t>(n));
    out_has_sibling = (sv.find(',') != std::string_view::npos) ||
                      (sv.find('-') != std::string_view::npos);
    return true;
}

std::vector<PhysicalCore> EnumerateLinuxPhysicalCores(bool& out_smt_detected) noexcept {
    std::vector<PhysicalCore> cores;
    out_smt_detected = false;
    std::uint64_t seen_primary_mask = 0;

    for (std::uint32_t cpu = 0; cpu < 64U; ++cpu) {
        std::uint32_t primary_id = cpu;
        bool has_sibling = false;
        if (!ParsePrimarySibling(cpu, primary_id, has_sibling)) {
            continue;
        }
        if (has_sibling) {
            out_smt_detected = true;
        }
        if (primary_id >= 64U) {
            continue;
        }
        const std::uint64_t bit = 1ULL << primary_id;
        if ((seen_primary_mask & bit) != 0) {
            continue; // Skip secondary SMT sibling thread
        }
        seen_primary_mask |= bit;

        // Read ARM/Intel hybrid capacity (1024 on P-cores / homogeneous x86, <1024 on E-cores)
        std::uint32_t capacity = 1024U;
        char cap_path[128]{};
        std::snprintf(cap_path, sizeof(cap_path),
                      "/sys/devices/system/cpu/cpu%u/cpu_capacity", primary_id);
        ReadSysfsUint(cap_path, capacity);

        cores.push_back(PhysicalCore{
            .primary_logical_cpu = primary_id,
            .efficiency_score = capacity,
            .has_smt_sibling = has_sibling,
        });
    }

    std::stable_sort(cores.begin(), cores.end(), [](const PhysicalCore& a, const PhysicalCore& b) {
        if (a.efficiency_score != b.efficiency_score) {
            return a.efficiency_score > b.efficiency_score;
        }
        return a.primary_logical_cpu < b.primary_logical_cpu;
    });

    return cores;
}

#endif

SiliconProfile ProbeHostSiliconOnce() noexcept {
    SiliconProfile profile{};

#ifdef _WIN32
    bool smt = false;
    const auto cores = EnumerateWin32PhysicalCores(smt);
    profile.smt_detected = smt;
    BuildProportionalMasks(cores, profile);
#elif defined(__linux__)
    bool smt = false;
    const auto cores = EnumerateLinuxPhysicalCores(smt);
    profile.smt_detected = smt;
    BuildProportionalMasks(cores, profile);

    const int msr_fd = ::open("/dev/cpu/0/msr", O_RDONLY | O_CLOEXEC);
    if (msr_fd >= 0) {
        std::uint64_t val = 0;
        if (::pread(msr_fd, &val, sizeof(val), 0xc8f) == sizeof(val)) {
            profile.cat_msr_available = true;
        }
        ::close(msr_fd);
    }
#else
    const unsigned int hw_threads = std::thread::hardware_concurrency();
    profile.smt_detected = (hw_threads > 6);
#endif

    return profile;
}

} // namespace

SiliconProfile DetectHostSilicon() noexcept {
    static std::once_flag probe_flag;
    static SiliconProfile cached_profile{};
    std::call_once(probe_flag, []() noexcept {
        cached_profile = ProbeHostSiliconOnce();
    });
    return cached_profile;
}

bool PinCurrentThread(Domain domain, int realtime_priority) noexcept {
    const SiliconProfile profile = DetectHostSilicon();
    std::uint64_t mask = profile.housekeeping_mask;
    switch (domain) {
        case Domain::AudioTiming:      mask = profile.audio_mask; break;
        case Domain::VulkanSubmission: mask = profile.vulkan_mask; break;
        case Domain::GuestJit:         mask = profile.jit_mask; break;
        case Domain::Housekeeping:     mask = profile.housekeeping_mask; break;
    }

    if (mask == 0) {
        return false;
    }

#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int cpu = 0; cpu < 64; ++cpu) {
        if ((mask & (1ULL << cpu)) != 0) {
            CPU_SET(cpu, &cpuset);
        }
    }
    if (::pthread_setaffinity_np(::pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        return false;
    }
    if (realtime_priority > 0) {
        const int min_prio = ::sched_get_priority_min(SCHED_FIFO);
        const int max_prio = ::sched_get_priority_max(SCHED_FIFO);
        sched_param param{};
        param.sched_priority = std::clamp(realtime_priority, min_prio, max_prio);
        if (::pthread_setschedparam(::pthread_self(), SCHED_FIFO, &param) != 0) {
            return false;
        }
    }
    return true;
#elif defined(_WIN32)
    const DWORD_PTR prev =
        ::SetThreadAffinityMask(::GetCurrentThread(), static_cast<DWORD_PTR>(mask));
    if (prev == 0) {
        return false;
    }
    if (domain == Domain::AudioTiming && realtime_priority > 0) {
        thread_local MmcssRegistration tl_mmcss;
        tl_mmcss.EnableProAudio();
    }
    return true;
#else
    (void)realtime_priority;
    return false;
#endif
}

} // namespace ax::topology
