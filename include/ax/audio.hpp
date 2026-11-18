#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace ax::audio {

constexpr std::uint32_t kTargetSampleRateHz = 48000;
constexpr std::uint32_t kTargetPeriodFrames = 96;   // 2.00 ms
constexpr std::uint32_t kFallbackPeriodFrames = 128; // 2.67 ms

template <std::size_t CapacityFrames = 2048>
class LockFreeStereoRing {
    static_assert((CapacityFrames & (CapacityFrames - 1)) == 0, "CapacityFrames must be power of 2");

public:
    std::size_t Push(const std::int32_t* interleaved_lr, std::size_t frames) noexcept {
        const std::size_t w = write_idx_.load(std::memory_order_relaxed);
        const std::size_t r = read_idx_.load(std::memory_order_acquire);
        const std::size_t avail = CapacityFrames - (w - r);
        const std::size_t to_write = (frames < avail) ? frames : avail;

        for (std::size_t i = 0; i < to_write; ++i) {
            const std::size_t slot = ((w + i) & (CapacityFrames - 1)) * 2;
            buffer_[slot] = interleaved_lr[i * 2];
            buffer_[slot + 1] = interleaved_lr[i * 2 + 1];
        }
        write_idx_.store(w + to_write, std::memory_order_release);
        return to_write;
    }

    std::size_t Pop(std::int32_t* out_interleaved_lr, std::size_t frames) noexcept {
        const std::size_t r = read_idx_.load(std::memory_order_relaxed);
        const std::size_t w = write_idx_.load(std::memory_order_acquire);
        const std::size_t avail = w - r;
        const std::size_t to_read = (frames < avail) ? frames : avail;

        for (std::size_t i = 0; i < to_read; ++i) {
            const std::size_t slot = ((r + i) & (CapacityFrames - 1)) * 2;
            out_interleaved_lr[i * 2] = buffer_[slot];
            out_interleaved_lr[i * 2 + 1] = buffer_[slot + 1];
        }
        read_idx_.store(r + to_read, std::memory_order_release);
        return to_read;
    }

private:
    alignas(64) std::atomic<std::size_t> write_idx_{0};
    alignas(64) std::atomic<std::size_t> read_idx_{0};
    alignas(64) std::array<std::int32_t, CapacityFrames * 2> buffer_{};
};

} // namespace ax::audio
