#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace ax {

inline constexpr std::uint32_t kTelemetryShmMagic   = 0x4158544Du; // "AXTM"
inline constexpr std::uint32_t kTelemetryShmVersion = 1u;

struct alignas(64) AudioJitterStats {
    static constexpr std::size_t kBucketWidthUs = 10u;
    static constexpr std::size_t kBucketCount   = 64u;

    std::atomic<std::uint32_t> min_callback_us{0xFFFFFFFFu};
    std::atomic<std::uint32_t> max_callback_us{0u};
    std::atomic<std::uint32_t> last_callback_us{0u};
    std::atomic<std::uint32_t> ring_low_water_frames{0xFFFFFFFFu};
    std::atomic<std::uint64_t> total_callbacks{0u};
    std::array<std::atomic<std::uint64_t>, kBucketCount> buckets{};
};

struct alignas(64) TelemetryCounters {
    std::atomic<std::uint32_t> magic{0u};
    std::atomic<std::uint32_t> version{0u};
    std::atomic<std::uint32_t> host_pid{0u};
    std::uint32_t reserved_0{0u};

    std::atomic<std::uint64_t> audio_underruns{0u};
    std::atomic<std::uint64_t> audio_buffer_switches{0u};
    std::atomic<std::uint32_t> audio_high_water_mark{0u};
    std::atomic<std::uint32_t> audio_processing_time_us{0u};
    std::atomic<std::uint32_t> voices_dropped{0u};
    std::uint32_t reserved_1{0u};

    std::atomic<std::uint64_t> vram_used_bytes{0u};
    std::atomic<std::uint64_t> vram_reaps{0u};
    std::atomic<std::uint64_t> bar1_stalls{0u};
    std::atomic<std::uint64_t> igpu_offloads{0u};
    std::atomic<std::uint64_t> qsv_frames_decoded{0u};

    std::atomic<std::uint64_t> frames_presented{0u};
    std::atomic<std::uint32_t> last_frametime_us{0u};
    std::atomic<std::uint32_t> working_set_mb{0u};
    std::atomic<std::uint32_t> commit_size_mb{0u};
    std::atomic<std::uint32_t> shader_stutters{0u};
    std::uint32_t reserved_2{0u};

    alignas(64) AudioJitterStats audio_jitter{};
};

struct TelemetrySnapshot {
    std::uint32_t host_pid{0u};

    std::uint64_t audio_underruns{0u};
    std::uint64_t audio_buffer_switches{0u};
    std::uint32_t audio_high_water_mark{0u};
    std::uint32_t audio_processing_time_us{0u};
    std::uint32_t voices_dropped{0u};

    std::uint32_t audio_jitter_min_us{0u};
    std::uint32_t audio_jitter_max_us{0u};
    std::uint32_t audio_jitter_p99_us{0u};
    std::uint32_t audio_ring_low_water_frames{0u};

    std::uint64_t vram_used_bytes{0u};
    std::uint64_t vram_reaps{0u};
    std::uint64_t bar1_stalls{0u};
    std::uint64_t igpu_offloads{0u};
    std::uint64_t qsv_frames_decoded{0u};

    std::uint64_t frames_presented{0u};
    std::uint32_t last_frametime_us{0u};
    std::uint32_t working_set_mb{0u};
    std::uint32_t commit_size_mb{0u};
    std::uint32_t shader_stutters{0u};
};

bool InitTelemetryShm() noexcept;
void ShutdownTelemetryShm() noexcept;
TelemetryCounters& GetTelemetry() noexcept;
void RecordAudioCallbackMetrics(std::uint32_t duration_us, std::uint32_t ring_fill_frames) noexcept;
TelemetrySnapshot GetTelemetrySnapshot() noexcept;
void ResetTelemetry() noexcept;

} // namespace ax
