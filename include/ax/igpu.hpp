#pragma once

#include <cstdint>

namespace ax::igpu {

constexpr std::uint64_t kDrmFormatModLinear = 0ULL;
constexpr double kMaxSharedDramBandwidthGBs = 3.50;

struct DmaBufSurfaceDesc {
    int dmabuf_fd = -1;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t plane_strides[2]{0, 0};
    std::uint32_t plane_offsets[2]{0, 0};
    std::uint64_t drm_modifier = kDrmFormatModLinear;
};

[[nodiscard]] inline bool IsBandwidthWithinGuardrail(double measured_gbs) noexcept {
    return measured_gbs <= kMaxSharedDramBandwidthGBs;
}

} // namespace ax::igpu
