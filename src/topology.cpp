#include "ax/topology.hpp"

#include <thread>

#ifdef __linux__
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <avrt.h>
#pragma comment(lib, "avrt.lib")
#endif

namespace ax::topology {

SiliconProfile DetectHostSilicon() noexcept {
    SiliconProfile profile{};
    const unsigned int hw_threads = std::thread::hardware_concurrency();
    if (hw_threads > 6) {
        profile.smt_detected = true;
    }

#ifdef __linux__
    const int msr_fd = ::open("/dev/cpu/0/msr", O_RDONLY);
    if (msr_fd >= 0) {
        std::uint64_t val = 0;
        if (::pread(msr_fd, &val, sizeof(val), 0xc8f) == sizeof(val)) {
            profile.cat_msr_available = true;
        }
        ::close(msr_fd);
    }
#endif
    return profile;
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
        sched_param param{};
        param.sched_priority = realtime_priority;
        ::pthread_setschedparam(::pthread_self(), SCHED_FIFO, &param);
    }
    return true;
#elif defined(_WIN32)
    const DWORD_PTR prev = ::SetThreadAffinityMask(::GetCurrentThread(), static_cast<DWORD_PTR>(mask));
    if (domain == Domain::AudioTiming && realtime_priority > 0) {
        DWORD task_index = 0;
        ::AvSetMmThreadCharacteristicsW(L"Pro Audio", &task_index);
    }
    return prev != 0;
#else
    (void)realtime_priority;
    return false;
#endif
}

} // namespace ax::topology
