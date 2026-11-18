#pragma once

#include <cstdint>

namespace ax::topology {

enum class Domain : std::uint8_t {
    AudioTiming = 0,      // Core 0 (0x01) — Unified L2 cache-local audio + CoreTiming
    VulkanSubmission = 1, // Core 1 (0x02) — Dedicated Vulkan command queue & dirty gather
    GuestJit = 2,         // Cores 2-4 (0x1C) — Tickless Guest CPU JIT execution
    Housekeeping = 3      // Core 5 (0x20) — Kernel IRQs, SPIR-V workers, FFmpeg slices, HID
};

struct SiliconProfile {
    std::uint64_t audio_mask = 0x01;
    std::uint64_t vulkan_mask = 0x02;
    std::uint64_t jit_mask = 0x1C;
    std::uint64_t housekeeping_mask = 0x20;
    bool smt_detected = false;
    bool cat_msr_available = false;
};

[[nodiscard]] SiliconProfile DetectHostSilicon() noexcept;
bool PinCurrentThread(Domain domain, int realtime_priority = 0) noexcept;

} // namespace ax::topology
