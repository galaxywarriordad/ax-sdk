#include "ax/config.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>

namespace ax {

namespace {

[[nodiscard]] std::string_view Trim(std::string_view sv) noexcept {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

[[nodiscard]] bool EqualsIgnoreCase(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto ca = static_cast<unsigned char>(a[i]);
        const auto cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool ParseBool(std::string_view val, bool fallback) noexcept {
    if (EqualsIgnoreCase(val, "true") || val == "1" || EqualsIgnoreCase(val, "yes")) {
        return true;
    }
    if (EqualsIgnoreCase(val, "false") || val == "0" || EqualsIgnoreCase(val, "no")) {
        return false;
    }
    return fallback;
}

template <typename IntT>
[[nodiscard]] IntT ParseInt(std::string_view val, IntT fallback) noexcept {
    IntT result{};
    const auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), result);
    return (ec == std::errc{}) ? result : fallback;
}

[[nodiscard]] FeatureOverride ParseFeatureOverride(std::string_view val, FeatureOverride fallback) noexcept {
    if (EqualsIgnoreCase(val, "auto")) {
        return FeatureOverride::Auto;
    }
    if (EqualsIgnoreCase(val, "force_on") || EqualsIgnoreCase(val, "true") || val == "1") {
        return FeatureOverride::ForceOn;
    }
    if (EqualsIgnoreCase(val, "force_off") || EqualsIgnoreCase(val, "false") || val == "0") {
        return FeatureOverride::ForceOff;
    }
    return fallback;
}

[[nodiscard]] ThreadPriorityTier ParsePriorityTier(std::string_view val, ThreadPriorityTier fallback) noexcept {
    if (EqualsIgnoreCase(val, "BELOWNORMAL")) {
        return ThreadPriorityTier::BelowNormal;
    }
    if (EqualsIgnoreCase(val, "NORMAL")) {
        return ThreadPriorityTier::Normal;
    }
    if (EqualsIgnoreCase(val, "ABOVENORMAL")) {
        return ThreadPriorityTier::AboveNormal;
    }
    if (EqualsIgnoreCase(val, "HIGH") || EqualsIgnoreCase(val, "REALTIME")) {
        // Clamp REALTIME to HIGH to protect Core 0 USB xHCI interrupts
        return ThreadPriorityTier::High;
    }
    if (EqualsIgnoreCase(val, "CRITICAL")) {
        return ThreadPriorityTier::Critical;
    }
    return fallback;
}

void ParseCoreRatio(std::string_view val, CoreRatioSpec& ratio) noexcept {
    const auto first_colon = val.find(':');
    if (first_colon == std::string_view::npos) {
        return;
    }
    const auto second_colon = val.find(':', first_colon + 1);
    if (second_colon == std::string_view::npos) {
        return;
    }
    ratio.audio_gpu = ParseInt(Trim(val.substr(0, first_colon)), ratio.audio_gpu);
    ratio.guest_jit = ParseInt(Trim(val.substr(first_colon + 1, second_colon - first_colon - 1)), ratio.guest_jit);
    ratio.aux_quarantine = ParseInt(Trim(val.substr(second_colon + 1)), ratio.aux_quarantine);
}

} // namespace

bool LoadEngineConfig(std::string_view ini_path, EngineConfig& out_config) noexcept {
    std::ifstream file{std::string(ini_path)};
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string current_section;

    while (std::getline(file, line)) {
        std::string_view view = Trim(line);
        if (view.empty() || view.front() == ';' || view.front() == '#') {
            continue;
        }

        if (view.front() == '[' && view.back() == ']') {
            current_section = std::string(Trim(view.substr(1, view.size() - 2)));
            continue;
        }

        const auto eq_pos = view.find('=');
        if (eq_pos == std::string_view::npos) {
            continue;
        }

        const std::string_view key = Trim(view.substr(0, eq_pos));
        std::string_view val = Trim(view.substr(eq_pos + 1));

        // Strip inline comments if present
        if (const auto comment_pos = val.find(';'); comment_pos != std::string_view::npos) {
            val = Trim(val.substr(0, comment_pos));
        }

        if (EqualsIgnoreCase(current_section, "EngineTopology")) {
            if (EqualsIgnoreCase(key, "EnableThreadAffinityMasks")) {
                out_config.enable_thread_affinity_masks = ParseBool(val, out_config.enable_thread_affinity_masks);
            } else if (EqualsIgnoreCase(key, "EnforceStrictCoreClamps")) {
                out_config.enforce_strict_core_clamps = ParseBool(val, out_config.enforce_strict_core_clamps);
            } else if (EqualsIgnoreCase(key, "MinCoresForIsolationSplit")) {
                out_config.min_cores_for_isolation_split = ParseInt(val, out_config.min_cores_for_isolation_split);
            } else if (EqualsIgnoreCase(key, "EnableProportionalScaler")) {
                out_config.enable_proportional_scaler = ParseBool(val, out_config.enable_proportional_scaler);
            } else if (EqualsIgnoreCase(key, "CoreRatio")) {
                ParseCoreRatio(val, out_config.core_ratio);
            } else if (EqualsIgnoreCase(key, "CoreRatioAudioGpu")) {
                out_config.core_ratio.audio_gpu = ParseInt(val, out_config.core_ratio.audio_gpu);
            } else if (EqualsIgnoreCase(key, "CoreRatioGuestJit")) {
                out_config.core_ratio.guest_jit = ParseInt(val, out_config.core_ratio.guest_jit);
            } else if (EqualsIgnoreCase(key, "CoreRatioAuxQuarantine")) {
                out_config.core_ratio.aux_quarantine = ParseInt(val, out_config.core_ratio.aux_quarantine);
            }
        } else if (EqualsIgnoreCase(current_section, "HardwareFeatureFlags")) {
            if (EqualsIgnoreCase(key, "MaxwellSafeUBOPassthrough")) {
                out_config.maxwell_safe_ubo_passthrough = ParseFeatureOverride(val, out_config.maxwell_safe_ubo_passthrough);
            } else if (EqualsIgnoreCase(key, "IntelQSVVideoOffload")) {
                out_config.intel_qsv_video_offload = ParseFeatureOverride(val, out_config.intel_qsv_video_offload);
            } else if (EqualsIgnoreCase(key, "VramShieldPruneTriggerMB")) {
                out_config.vram_shield_prune_trigger_mb = ParseInt(val, out_config.vram_shield_prune_trigger_mb);
            } else if (EqualsIgnoreCase(key, "VramShieldCeilingMB")) {
                out_config.vram_shield_ceiling_mb = ParseInt(val, out_config.vram_shield_ceiling_mb);
            } else if (EqualsIgnoreCase(key, "VramShieldLsfgCeilingMB")) {
                out_config.vram_shield_lsfg_ceiling_mb = ParseInt(val, out_config.vram_shield_lsfg_ceiling_mb);
            } else if (EqualsIgnoreCase(key, "Bar1StagingCapMB")) {
                out_config.bar1_staging_cap_mb = ParseInt(val, out_config.bar1_staging_cap_mb);
            }
        } else if (EqualsIgnoreCase(current_section, "AudioPipeline")) {
            if (EqualsIgnoreCase(key, "ExclusiveDeviceName")) {
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                    val = val.substr(1, val.size() - 2);
                }
                out_config.exclusive_device_name = std::string(val);
            } else if (EqualsIgnoreCase(key, "TargetSampleRateHz")) {
                out_config.target_sample_rate_hz = ParseInt(val, out_config.target_sample_rate_hz);
            } else if (EqualsIgnoreCase(key, "AsioBufferSamples")) {
                out_config.asio_buffer_samples = ParseInt(val, out_config.asio_buffer_samples);
            } else if (EqualsIgnoreCase(key, "AlsaPeriodFrames")) {
                out_config.alsa_period_frames = ParseInt(val, out_config.alsa_period_frames);
            } else if (EqualsIgnoreCase(key, "EnableMicroPll")) {
                out_config.enable_micro_pll = ParseBool(val, out_config.enable_micro_pll);
            }
        } else if (EqualsIgnoreCase(current_section, "Telemetry")) {
            if (EqualsIgnoreCase(key, "EnableSharedMemoryExport")) {
                out_config.enable_shared_memory_export = ParseBool(val, out_config.enable_shared_memory_export);
            } else if (EqualsIgnoreCase(key, "OsdOverlayEnabled")) {
                out_config.osd_overlay_enabled = ParseBool(val, out_config.osd_overlay_enabled);
            } else if (EqualsIgnoreCase(key, "SampleIntervalMs")) {
                out_config.sample_interval_ms = ParseInt(val, out_config.sample_interval_ms);
            }
        } else if (EqualsIgnoreCase(current_section, "WindowsPlatformHacks")) {
            if (EqualsIgnoreCase(key, "EnableMMCSSProAudioScaling")) {
                out_config.enable_mmcss_pro_audio_scaling = ParseBool(val, out_config.enable_mmcss_pro_audio_scaling);
            } else if (EqualsIgnoreCase(key, "HostTimingPriorityClass")) {
                out_config.host_timing_priority = ParsePriorityTier(val, out_config.host_timing_priority);
            } else if (EqualsIgnoreCase(key, "VulkanDriverPriorityClass")) {
                out_config.vulkan_driver_priority = ParsePriorityTier(val, out_config.vulkan_driver_priority);
            } else if (EqualsIgnoreCase(key, "GuestExecutionPriorityClass")) {
                out_config.guest_execution_priority = ParsePriorityTier(val, out_config.guest_execution_priority);
            } else if (EqualsIgnoreCase(key, "AuxQuarantinePriorityClass")) {
                out_config.aux_quarantine_priority = ParsePriorityTier(val, out_config.aux_quarantine_priority);
            }
        }
    }

    return true;
}

bool WriteDefaultEngineConfig(std::string_view ini_path) noexcept {
    std::ofstream file{std::string(ini_path)};
    if (!file.is_open()) {
        return false;
    }
    file << "; =============================================================================\n"
         << "; [AX] Unified Engine & Silicon Configuration (ax_engine.ini)\n"
         << "; Shared across Eden-Espresso-Wave-AX, Cemu-Latte-AX, Piccolo-AX & FLAC_AX\n"
         << "; =============================================================================\n\n"
         << "[EngineTopology]\n"
         << "EnableThreadAffinityMasks=true\n"
         << "EnforceStrictCoreClamps=true\n"
         << "MinCoresForIsolationSplit=6\n"
         << "EnableProportionalScaler=true\n"
         << "CoreRatio=2:3:1\n\n"
         << "[HardwareFeatureFlags]\n"
         << "MaxwellSafeUBOPassthrough=auto\n"
         << "IntelQSVVideoOffload=auto\n"
         << "VramShieldPruneTriggerMB=5000\n"
         << "VramShieldCeilingMB=5500\n"
         << "VramShieldLsfgCeilingMB=5350\n"
         << "Bar1StagingCapMB=128\n\n"
         << "[AudioPipeline]\n"
         << "ExclusiveDeviceName=\n"
         << "TargetSampleRateHz=48000\n"
         << "AsioBufferSamples=128\n"
         << "AlsaPeriodFrames=96\n"
         << "EnableMicroPll=true\n\n"
         << "[Telemetry]\n"
         << "EnableSharedMemoryExport=true\n"
         << "OsdOverlayEnabled=false\n"
         << "SampleIntervalMs=250\n\n"
         << "[WindowsPlatformHacks]\n"
         << "EnableMMCSSProAudioScaling=true\n"
         << "HostTimingPriorityClass=HIGH\n"
         << "VulkanDriverPriorityClass=HIGH\n"
         << "GuestExecutionPriorityClass=ABOVENORMAL\n"
         << "AuxQuarantinePriorityClass=BELOWNORMAL\n";

    return file.good();
}

} // namespace ax

