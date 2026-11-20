#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace ax {

inline constexpr std::uint32_t kTelemetryShmMagic   = 0x4158544D; // "AXTM"
inline constexpr std::uint32_t kTelemetryShmVersion = 1;

// -----------------------------------------------------------------------------
// Lock-Free Microsecond Audio Callback Jitter Histogram (Core 0 RT Safe)
// 16 buckets covering 0..4000+ us in 250 us steps + exact min/max/last
// -----------------------------------------------------------------------------
struct alignas(64) AudioJitterStats {
    static constexpr std::size_t kBucketCount = 16;
    static constexpr std::uint32_t kBucketWidthUs = 250;

    std::atomic<std::uint32_t> min_callback_us{0xFFFFFFFFu};
    std::atomic<std::uint32_t> max_callback_us{0};
    std::atomic<std::uint32_t> last_callback_us{0};
    std::atomic<std::uint32_t> ring_low_water_frames{0xFFFFFFFFu};
    std::atomic<std::uint64_t> total_callbacks{0};
    std::array<std::atomic<std::uint64_t>, kBucketCount> buckets{};
};

// -----------------------------------------------------------------------------
// Cache-Line-Padded Atomic Telemetry Block (AX-1.3 / v1.36.1)
// Prevents false sharing across Core 0 (Audio), Core 1 (Vulkan), and Core 5 (Aux)
// -----------------------------------------------------------------------------
struct alignas(64) TelemetryCounters {
    // Shared Memory Header (Cache Line 0)
    alignas(64) std::atomic<std::uint32_t> magic{kTelemetryShmMagic};
    std::atomic<std::uint32_t> version{kTelemetryShmVersion};
    std::atomic<std::uint32_t> host_pid{0};
    std::atomic<std::uint32_t> reserved{0};

    // Cache Line 1: Core 0 Audio & Timing Domain
    alignas(64) std::atomic<std::uint64_t> audio_underruns{0};
    std::atomic<std::uint64_t> audio_buffer_switches{0};
    std::atomic<std::uint32_t> audio_high_water_mark{0};
    std::atomic<std::uint32_t> audio_processing_time_us{0};
    std::atomic<std::uint32_t> voices_dropped{0};

    // Cache Line 2: Core 1 & Core 5 GPU / VRAM / BAR1 Domain
    alignas(64) std::atomic<std::uint64_t> vram_used_bytes{0};
    std::atomic<std::uint64_t> vram_reaps{0};
    std::atomic<std::uint64_t> bar1_stalls{0};
    std::atomic<std::uint64_t> igpu_offloads{0};
    std::atomic<std::uint64_t> qsv_frames_decoded{0};

    // Cache Line 3: Frame Pacing & Memory Footprint
    alignas(64) std::atomic<std::uint64_t> frames_presented{0};
    std::atomic<std::uint32_t> last_frametime_us{0};
    std::atomic<std::uint32_t> working_set_mb{0};
    std::atomic<std::uint32_t> commit_size_mb{0};
    std::atomic<std::uint32_t> shader_stutters{0};

    // Cache Lines 4+: Audio Callback Jitter Histogram
    AudioJitterStats audio_jitter{};
};

// Plain-Old-Data (POD) snapshot for zero-lock OSD rendering and JSON serialization
struct TelemetrySnapshot {
    std::uint32_t host_pid{0};

    std::uint64_t audio_underruns{0};
    std::uint64_t audio_buffer_switches{0};
    std::uint32_t audio_high_water_mark{0};
    std::uint32_t audio_processing_time_us{0};
    std::uint32_t voices_dropped{0};

    std::uint32_t audio_jitter_min_us{0};
    std::uint32_t audio_jitter_p99_us{0};
    std::uint32_t audio_jitter_max_us{0};
    std::uint32_t audio_ring_low_water_frames{0};

    std::uint64_t vram_used_bytes{0};
    std::uint64_t vram_reaps{0};
    std::uint64_t bar1_stalls{0};
    std::uint64_t igpu_offloads{0};
    std::uint64_t qsv_frames_decoded{0};

    std::uint64_t frames_presented{0};
    std::uint32_t last_frametime_us{0};
    std::uint32_t working_set_mb{0};
    std::uint32_t commit_size_mb{0};
    std::uint32_t shader_stutters{0};
};

// Maps TelemetryCounters into cross-process shared memory (Local\AX_Telemetry or /dev/shm/ax_telemetry).
// Safe to call at startup; falls back transparently to process-local storage if SHM is unavailable.
bool InitTelemetryShm() noexcept;

// Unmaps and closes shared memory handles cleanly at shutdown.
void ShutdownTelemetryShm() noexcept;

// Global telemetry block (backed by shared memory when InitTelemetryShm() succeeds)
[[nodiscard]] TelemetryCounters& GetTelemetry() noexcept;

// RT-safe Core 0 helper: records audio callback duration (us) and current ring fill level (frames)
void RecordAudioCallbackMetrics(std::uint32_t duration_us, std::uint32_t ring_fill_frames) noexcept;

// Lock-free snapshot copy (computes P99 audio jitter from histogram buckets)
[[nodiscard]] TelemetrySnapshot GetTelemetrySnapshot() noexcept;

// Resets all counters and histogram buckets to zero (typically called at title/track launch)
void ResetTelemetry() noexcept;

} // namespace ax
