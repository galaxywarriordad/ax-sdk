#pragma once

#include <cstdint>

namespace ax::vram {

constexpr std::uint64_t kDefaultCeilingBytes = 5905580032ULL; // 5.50 GiB
constexpr std::uint64_t kLsfgCeilingBytes    = 5744518758ULL; // 5.35 GiB

struct SiliconCapabilities {
    bool is_maxwell_gm200 = false;
    bool promote_fp16_to_fp32 = false;
    bool has_dedicated_transfer_queue = false;
    bool has_bar1_wc_pool = false;
    bool prefer_subgroup_shfl = false;
    bool is_igpu_vk13_capped = false;
};

struct BudgetSnapshot {
    std::uint64_t current_usage_bytes = 0;
    std::uint64_t active_ceiling_bytes = kDefaultCeilingBytes;
    bool eviction_required = false;
};

[[nodiscard]] inline BudgetSnapshot EvaluateBudget(std::uint64_t current_usage_bytes,
                                                   bool lsfg_active) noexcept {
    const std::uint64_t ceiling = lsfg_active ? kLsfgCeilingBytes : kDefaultCeilingBytes;
    return BudgetSnapshot{
        .current_usage_bytes = current_usage_bytes,
        .active_ceiling_bytes = ceiling,
        .eviction_required = current_usage_bytes >= ceiling
    };
}

} // namespace ax::vram
