#include "ax/telemetry.hpp"
#include "ax/config.hpp"

#include <algorithm>
#include <cstring>
#include <new>

#if AX_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif AX_PLATFORM_LINUX
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

namespace ax {

namespace {

alignas(64) TelemetryCounters g_local_fallback{};
TelemetryCounters* g_active_telemetry = &g_local_fallback;

#if AX_PLATFORM_WINDOWS
HANDLE g_shm_handle = nullptr;
void*  g_shm_view   = nullptr;
#elif AX_PLATFORM_LINUX
int    g_shm_fd     = -1;
void*  g_shm_view   = nullptr;
#endif

void InitializeHeader(TelemetryCounters* ptr) noexcept {
    ptr->magic.store(kTelemetryShmMagic, std::memory_order_relaxed);
    ptr->version.store(kTelemetryShmVersion, std::memory_order_relaxed);
#if AX_PLATFORM_WINDOWS
    ptr->host_pid.store(static_cast<std::uint32_t>(::GetCurrentProcessId()), std::memory_order_release);
#elif AX_PLATFORM_LINUX
    ptr->host_pid.store(static_cast<std::uint32_t>(::getpid()), std::memory_order_release);
#endif
}

template <typename T>
void AtomicFetchMin(std::atomic<T>& target, T value) noexcept {
    T prev = target.load(std::memory_order_relaxed);
    while (value < prev && !target.compare_exchange_weak(prev, value, std::memory_order_relaxed)) {
    }
}

template <typename T>
void AtomicFetchMax(std::atomic<T>& target, T value) noexcept {
    T prev = target.load(std::memory_order_relaxed);
    while (value > prev && !target.compare_exchange_weak(prev, value, std::memory_order_relaxed)) {
    }
}

} // namespace

bool InitTelemetryShm() noexcept {
    if (g_shm_view != nullptr) {
        return true;
    }

    constexpr std::size_t kMapSize = sizeof(TelemetryCounters);

#if AX_PLATFORM_WINDOWS
    const DWORD size_hi = static_cast<DWORD>((static_cast<std::uint64_t>(kMapSize) >> 32) & 0xFFFFFFFFull);
    const DWORD size_lo = static_cast<DWORD>(kMapSize & 0xFFFFFFFFull);

    g_shm_handle = ::CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        size_hi,
        size_lo,
        "Local\\AX_Telemetry"
    );
    if (g_shm_handle == nullptr) {
        return false;
    }

    const bool already_existed = (::GetLastError() == ERROR_ALREADY_EXISTS);
    g_shm_view = ::MapViewOfFile(g_shm_handle, FILE_MAP_ALL_ACCESS, 0, 0, kMapSize);
    if (g_shm_view == nullptr) {
        ::CloseHandle(g_shm_handle);
        g_shm_handle = nullptr;
        return false;
    }

    if (!already_existed) {
        new (g_shm_view) TelemetryCounters{};
    }
    g_active_telemetry = static_cast<TelemetryCounters*>(g_shm_view);
    InitializeHeader(g_active_telemetry);
    return true;

#elif AX_PLATFORM_LINUX
    g_shm_fd = ::shm_open("/ax_telemetry", O_CREAT | O_RDWR, 0666);
    if (g_shm_fd < 0) {
        return false;
    }

    if (::ftruncate(g_shm_fd, static_cast<off_t>(kMapSize)) != 0) {
        ::close(g_shm_fd);
        g_shm_fd = -1;
        return false;
    }

    g_shm_view = ::mmap(nullptr, kMapSize, PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
    if (g_shm_view == MAP_FAILED) {
        g_shm_view = nullptr;
        ::close(g_shm_fd);
        g_shm_fd = -1;
        return false;
    }

    auto* candidate = static_cast<TelemetryCounters*>(g_shm_view);
    if (candidate->magic.load(std::memory_order_acquire) != kTelemetryShmMagic) {
        new (g_shm_view) TelemetryCounters{};
    }
    g_active_telemetry = candidate;
    InitializeHeader(g_active_telemetry);
    return true;
#else
    return false;
#endif
}

void ShutdownTelemetryShm() noexcept {
    g_active_telemetry = &g_local_fallback;

#if AX_PLATFORM_WINDOWS
    if (g_shm_view != nullptr) {
        ::UnmapViewOfFile(g_shm_view);
        g_shm_view = nullptr;
    }
    if (g_shm_handle != nullptr) {
        ::CloseHandle(g_shm_handle);
        g_shm_handle = nullptr;
    }
#elif AX_PLATFORM_LINUX
    if (g_shm_view != nullptr) {
        ::munmap(g_shm_view, sizeof(TelemetryCounters));
        g_shm_view = nullptr;
    }
    if (g_shm_fd >= 0) {
        ::close(g_shm_fd);
        g_shm_fd = -1;
    }
#endif
}

TelemetryCounters& GetTelemetry() noexcept {
    return *g_active_telemetry;
}

void RecordAudioCallbackMetrics(std::uint32_t duration_us, std::uint32_t ring_fill_frames) noexcept {
    auto& t = GetTelemetry();
    t.audio_processing_time_us.store(duration_us, std::memory_order_relaxed);
    AtomicFetchMax(t.audio_high_water_mark, ring_fill_frames);

    auto& j = t.audio_jitter;
    j.last_callback_us.store(duration_us, std::memory_order_relaxed);
    AtomicFetchMin(j.min_callback_us, duration_us);
    AtomicFetchMax(j.max_callback_us, duration_us);
    AtomicFetchMin(j.ring_low_water_frames, ring_fill_frames);
    j.total_callbacks.fetch_add(1, std::memory_order_relaxed);

    const std::size_t bucket_idx = std::min<std::size_t>(
        duration_us / AudioJitterStats::kBucketWidthUs,
        AudioJitterStats::kBucketCount - 1
    );
    j.buckets[bucket_idx].fetch_add(1, std::memory_order_relaxed);
}

TelemetrySnapshot GetTelemetrySnapshot() noexcept {
    const auto& t = GetTelemetry();
    TelemetrySnapshot snap{};
    snap.host_pid = t.host_pid.load(std::memory_order_acquire);

    snap.audio_underruns = t.audio_underruns.load(std::memory_order_acquire);
    snap.audio_buffer_switches = t.audio_buffer_switches.load(std::memory_order_acquire);
    snap.audio_high_water_mark = t.audio_high_water_mark.load(std::memory_order_relaxed);
    snap.audio_processing_time_us = t.audio_processing_time_us.load(std::memory_order_relaxed);
    snap.voices_dropped = t.voices_dropped.load(std::memory_order_acquire);

    const auto& j = t.audio_jitter;
    const std::uint32_t raw_min = j.min_callback_us.load(std::memory_order_relaxed);
    snap.audio_jitter_min_us = (raw_min == 0xFFFFFFFFu) ? 0u : raw_min;
    snap.audio_jitter_max_us = j.max_callback_us.load(std::memory_order_relaxed);

    const std::uint32_t raw_low = j.ring_low_water_frames.load(std::memory_order_relaxed);
    snap.audio_ring_low_water_frames = (raw_low == 0xFFFFFFFFu) ? 0u : raw_low;

    const std::uint64_t total_cb = j.total_callbacks.load(std::memory_order_relaxed);
    if (total_cb > 0) {
        const std::uint64_t p99_target = (total_cb * 99u + 99u) / 100u;
        std::uint64_t cumulative = 0;
        for (std::size_t i = 0; i < AudioJitterStats::kBucketCount; ++i) {
            cumulative += j.buckets[i].load(std::memory_order_relaxed);
            if (cumulative >= p99_target) {
                snap.audio_jitter_p99_us = static_cast<std::uint32_t>((i + 1) * AudioJitterStats::kBucketWidthUs);
                break;
            }
        }
    }

    snap.vram_used_bytes = t.vram_used_bytes.load(std::memory_order_acquire);
    snap.vram_reaps = t.vram_reaps.load(std::memory_order_relaxed);
    snap.bar1_stalls = t.bar1_stalls.load(std::memory_order_relaxed);
    snap.igpu_offloads = t.igpu_offloads.load(std::memory_order_relaxed);
    snap.qsv_frames_decoded = t.qsv_frames_decoded.load(std::memory_order_relaxed);

    snap.frames_presented = t.frames_presented.load(std::memory_order_relaxed);
    snap.last_frametime_us = t.last_frametime_us.load(std::memory_order_relaxed);
    snap.working_set_mb = t.working_set_mb.load(std::memory_order_relaxed);
    snap.commit_size_mb = t.commit_size_mb.load(std::memory_order_relaxed);
    snap.shader_stutters = t.shader_stutters.load(std::memory_order_relaxed);
    return snap;
}

void ResetTelemetry() noexcept {
    auto& t = GetTelemetry();
    t.audio_underruns.store(0, std::memory_order_relaxed);
    t.audio_buffer_switches.store(0, std::memory_order_relaxed);
    t.audio_high_water_mark.store(0, std::memory_order_relaxed);
    t.audio_processing_time_us.store(0, std::memory_order_relaxed);
    t.voices_dropped.store(0, std::memory_order_relaxed);

    auto& j = t.audio_jitter;
    j.min_callback_us.store(0xFFFFFFFFu, std::memory_order_relaxed);
    j.max_callback_us.store(0, std::memory_order_relaxed);
    j.last_callback_us.store(0, std::memory_order_relaxed);
    j.ring_low_water_frames.store(0xFFFFFFFFu, std::memory_order_relaxed);
    j.total_callbacks.store(0, std::memory_order_relaxed);
    for (auto& b : j.buckets) {
        b.store(0, std::memory_order_relaxed);
    }

    t.vram_used_bytes.store(0, std::memory_order_relaxed);
    t.vram_reaps.store(0, std::memory_order_relaxed);
    t.bar1_stalls.store(0, std::memory_order_relaxed);
    t.igpu_offloads.store(0, std::memory_order_relaxed);
    t.qsv_frames_decoded.store(0, std::memory_order_relaxed);

    t.frames_presented.store(0, std::memory_order_relaxed);
    t.last_frametime_us.store(0, std::memory_order_relaxed);
    t.working_set_mb.store(0, std::memory_order_relaxed);
    t.commit_size_mb.store(0, std::memory_order_relaxed);
    t.shader_stutters.store(0, std::memory_order_relaxed);
}

} // namespace ax
