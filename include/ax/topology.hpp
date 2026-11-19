#pragma once

#include <cstdint>

namespace ax::topology {

enum class Domain : std::uint8_t {
    AudioTiming = 0,
    VulkanSubmission = 1,
    GuestJit = 2,
    Housekeeping = 3,
};

struct SiliconProfile {
    std::uint64_t audio_mask = 0x01;        // Core 0: Audio & Master Timing
    std::uint64_t vulkan_mask = 0x02;       // Core 1: Vulkan Command Submission
    std::uint64_t jit_mask = 0x1C;          // Cores 2–4: Home-Biased Guest ARM64 JIT
    std::uint64_t housekeeping_mask = 0x20; // Core 5: Quarantine Sink (VRAM Reaper / SPIR-V / I/O / HID)
    bool smt_detected = false;
    bool cat_msr_available = false;
};

[[nodiscard]] SiliconProfile DetectHostSilicon() noexcept;
bool PinCurrentThread(Domain domain, int realtime_priority = 0) noexcept;

} // namespace ax::topology
