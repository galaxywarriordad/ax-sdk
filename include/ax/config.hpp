#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace ax {

enum class FeatureOverride : std::uint8_t {
	Auto = 0,
	Enabled = 1,
	Disabled = 2,
};

enum class ThreadPriorityTier : std::uint8_t {
	Normal = 0,
	Elevated = 1,
	RealtimeAudio = 2,
	Housekeeping = 3,
};

struct CoreRatioSpec {
	int audio_gpu = 2;
	int guest_jit = 3;
	int aux_quarantine = 1;
};

struct EngineConfig {
	bool enable_thread_affinity_masks = true;
	bool enforce_strict_core_clamps = true;
	int min_cores_for_isolation_split = 4;
	bool enable_proportional_scaler = true;

	CoreRatioSpec core_ratio{};

	FeatureOverride maxwell_safe_ubo_passthrough = FeatureOverride::Auto;
	FeatureOverride intel_qsv_video_offload = FeatureOverride::Auto;

	int vram_shield_prune_trigger_mb = 5400;
	int vram_shield_ceiling_mb = 5632;
	int vram_shield_lsfg_ceiling_mb = 5120;
	int bar1_staging_cap_mb = 256;

	ThreadPriorityTier audio_priority_tier = ThreadPriorityTier::RealtimeAudio;
	ThreadPriorityTier housekeeping_priority_tier = ThreadPriorityTier::Housekeeping;
};

[[nodiscard]] bool LoadEngineConfig(std::string_view path, EngineConfig& out_config) noexcept;

} // namespace ax
