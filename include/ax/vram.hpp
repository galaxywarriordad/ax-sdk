#pragma once

#include <cstdint>

namespace ax::vram {

constexpr std::uint64_t kPruneTriggerBytes        = 5368709120ULL; // 5.00 GiB async prune watermark
constexpr std::uint64_t kDefaultCeilingBytes      = 5905580032ULL; // 5.50 GiB hard ceiling
constexpr std::uint64_t kLsfgCeilingBytes         = 5744518758ULL; // 5.35 GiB LSFG ceiling
constexpr std::uint64_t kWin32LsCeilingBytes      = 5583457485ULL; // 5.20 GiB Win32 LS ceiling
constexpr double        kDefaultEvictionSlice     = 0.10;          // 10% LRU batch per async pass

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
    std::uint64_t recovery_target_bytes = 0;
    double        eviction_ratio = 0.0;
    bool          eviction_required = false;
    bool          force_lod_downscale = false;
};

[[nodiscard]] inline BudgetSnapshot EvaluateBudget(std::uint64_t current_usage_bytes,
                                                   bool lsfg_active,
                                                   bool eviction_in_flight = false) noexcept {
    const std::uint64_t ceiling = lsfg_active ? kLsfgCeilingBytes : kDefaultCeilingBytes;
    const std::uint64_t recovery_target = (kPruneTriggerBytes * 90ULL) / 100ULL; // ~4.50 GiB low-water mark

    const bool over_prune_trigger = current_usage_bytes >= kPruneTriggerBytes;
    const bool should_dispatch = over_prune_trigger && !eviction_in_flight;

    return BudgetSnapshot{
        .current_usage_bytes = current_usage_bytes,
        .active_ceiling_bytes = ceiling,
        .recovery_target_bytes = recovery_target,
        .eviction_ratio = should_dispatch ? kDefaultEvictionSlice : 0.0,
        .eviction_required = should_dispatch,
        .force_lod_downscale = current_usage_bytes >= ceiling,
    };
}

} // namespace ax::vram
