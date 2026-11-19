#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace ax::audio {

constexpr std::uint32_t kTargetSampleRateHz = 48000;
constexpr std::uint32_t kTargetPeriodFrames = 96;    // 2.00 ms (Piccolo ALSA default)
constexpr std::uint32_t kFallbackPeriodFrames = 128; // 2.67 ms (Win32 ASIO / ALSA fallback)

template <std::size_t CapacityFrames = 2048, typename SampleT = std::int32_t>
class LockFreeStereoRing {
    static_assert((CapacityFrames & (CapacityFrames - 1)) == 0,
                  "CapacityFrames must be a power of 2");
    static_assert(std::is_trivially_copyable_v<SampleT>,
                  "SampleT must be trivially copyable");

public:
    static constexpr std::size_t kMask = CapacityFrames - 1;

    [[nodiscard]] std::size_t AvailableFrames() const noexcept {
        const std::size_t w = write_idx_.load(std::memory_order_acquire);
        const std::size_t r = read_idx_.load(std::memory_order_acquire);
        return w - r;
    }

    [[nodiscard]] std::size_t FreeFrames() const noexcept {
        return CapacityFrames - AvailableFrames();
    }

    /// Single-producer batch push of interleaved stereo [L, R] frames using contiguous memcpy.
    std::size_t Push(const SampleT* interleaved_lr, std::size_t frames) noexcept {
        if (interleaved_lr == nullptr || frames == 0) {
            return 0;
        }
        const std::size_t w = write_idx_.load(std::memory_order_relaxed);
        const std::size_t r = read_idx_.load(std::memory_order_acquire);
        const std::size_t avail = CapacityFrames - (w - r);
        const std::size_t to_write = (frames < avail) ? frames : avail;
        if (to_write == 0) {
            return 0;
        }

        const std::size_t start_frame = w & kMask;
        const std::size_t first_frames =
            ((CapacityFrames - start_frame) < to_write) ? (CapacityFrames - start_frame) : to_write;
        const std::size_t second_frames = to_write - first_frames;

        std::memcpy(&buffer_[start_frame * 2], interleaved_lr,
                    first_frames * 2 * sizeof(SampleT));
        if (second_frames > 0) {
            std::memcpy(&buffer_[0], interleaved_lr + (first_frames * 2),
                        second_frames * 2 * sizeof(SampleT));
        }

        write_idx_.store(w + to_write, std::memory_order_release);
        return to_write;
    }

    /// Single-consumer batch pop into interleaved stereo [L, R] frames (ALSA / WASAPI).
    std::size_t Pop(SampleT* out_interleaved_lr, std::size_t frames) noexcept {
        if (out_interleaved_lr == nullptr || frames == 0) {
            return 0;
        }
        const std::size_t r = read_idx_.load(std::memory_order_relaxed);
        const std::size_t w = write_idx_.load(std::memory_order_acquire);
        const std::size_t avail = w - r;
        const std::size_t to_read = (frames < avail) ? frames : avail;
        if (to_read == 0) {
            return 0;
        }

        const std::size_t start_frame = r & kMask;
        const std::size_t first_frames =
            ((CapacityFrames - start_frame) < to_read) ? (CapacityFrames - start_frame) : to_read;
        const std::size_t second_frames = to_read - first_frames;

        std::memcpy(out_interleaved_lr, &buffer_[start_frame * 2],
                    first_frames * 2 * sizeof(SampleT));
        if (second_frames > 0) {
            std::memcpy(out_interleaved_lr + (first_frames * 2), &buffer_[0],
                        second_frames * 2 * sizeof(SampleT));
        }

        read_idx_.store(r + to_read, std::memory_order_release);
        return to_read;
    }

    /// Single-consumer batch pop de-interleaved into separate Left/Right buffers (Steinberg ASIO).
    std::size_t PopPlanar(SampleT* out_left, SampleT* out_right, std::size_t frames) noexcept {
        if (out_left == nullptr || out_right == nullptr || frames == 0) {
            return 0;
        }
        const std::size_t r = read_idx_.load(std::memory_order_relaxed);
        const std::size_t w = write_idx_.load(std::memory_order_acquire);
        const std::size_t avail = w - r;
        const std::size_t to_read = (frames < avail) ? frames : avail;
        if (to_read == 0) {
            return 0;
        }

        for (std::size_t i = 0; i < to_read; ++i) {
            const std::size_t slot = ((r + i) & kMask) * 2;
            out_left[i] = buffer_[slot];
            out_right[i] = buffer_[slot + 1];
        }

        read_idx_.store(r + to_read, std::memory_order_release);
        return to_read;
    }

private:
    alignas(64) std::atomic<std::size_t> write_idx_{0};
    char pad_write_[64 - sizeof(std::atomic<std::size_t>)]{};
    alignas(64) std::atomic<std::size_t> read_idx_{0};
    char pad_read_[64 - sizeof(std::atomic<std::size_t>)]{};
    alignas(64) std::array<SampleT, CapacityFrames * 2> buffer_{};
};

} // namespace ax::audio
