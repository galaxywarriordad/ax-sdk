#include "ax/config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>

namespace ax {

namespace {

std::string_view Trim(std::string_view sv) noexcept {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

std::string ToLowerAscii(std::string_view sv) {
    std::string out(sv);
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

bool ParseBoolValue(std::string_view raw, bool fallback) noexcept {
    const std::string lower = ToLowerAscii(Trim(raw));
    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") {
        return true;
    }
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off") {
        return false;
    }
    return fallback;
}

int ParseIntValue(std::string_view raw, int fallback) noexcept {
    const std::string trimmed(Trim(raw));
    if (trimmed.empty()) {
        return fallback;
    }
    char* end_ptr = nullptr;
    const long val = std::strtol(trimmed.c_str(), &end_ptr, 10);
    if (end_ptr == trimmed.c_str() || *end_ptr != '\0') {
        return fallback;
    }
    return static_cast<int>(val);
}

FeatureOverride ParseFeatureOverrideValue(std::string_view raw, FeatureOverride fallback) noexcept {
    const std::string lower = ToLowerAscii(Trim(raw));
    if (lower == "auto") {
        return FeatureOverride::Auto;
    }
    if (lower == "enabled" || lower == "enable" || lower == "on" || lower == "true" || lower == "1") {
        return FeatureOverride::Enabled;
    }
    if (lower == "disabled" || lower == "disable" || lower == "off" || lower == "false" || lower == "0") {
        return FeatureOverride::Disabled;
    }
    return fallback;
}

ThreadPriorityTier ParsePriorityTierValue(std::string_view raw, ThreadPriorityTier fallback) noexcept {
    const std::string lower = ToLowerAscii(Trim(raw));
    if (lower == "normal") {
        return ThreadPriorityTier::Normal;
    }
    if (lower == "elevated" || lower == "abovenormal" || lower == "high") {
        return ThreadPriorityTier::Elevated;
    }
    if (lower == "realtimeaudio" || lower == "realtime_audio" || lower == "rt" || lower == "critical") {
        return ThreadPriorityTier::RealtimeAudio;
    }
    if (lower == "housekeeping" || lower == "low" || lower == "belownormal" || lower == "aux") {
        return ThreadPriorityTier::Housekeeping;
    }
    return fallback;
}

void ParseCoreRatioValue(std::string_view raw, CoreRatioSpec& ratio) noexcept {
    const std::string_view clean = Trim(raw);
    const std::size_t first_colon = clean.find(':');
    if (first_colon == std::string_view::npos) {
        return;
    }
    const std::size_t second_colon = clean.find(':', first_colon + 1);
    if (second_colon == std::string_view::npos) {
        return;
    }

    const int a = ParseIntValue(clean.substr(0, first_colon), ratio.audio_gpu);
    const int b = ParseIntValue(clean.substr(first_colon + 1, second_colon - (first_colon + 1)), ratio.guest_jit);
    const int c = ParseIntValue(clean.substr(second_colon + 1), ratio.aux_quarantine);

    if (a > 0 && b > 0 && c > 0) {
        ratio.audio_gpu = a;
        ratio.guest_jit = b;
        ratio.aux_quarantine = c;
    }
}

} // namespace

bool LoadEngineConfig(std::string_view path, EngineConfig& out_config) noexcept {
    out_config = EngineConfig{};
    if (path.empty()) {
        return false;
    }

    std::ifstream file{std::string(path)};
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string_view sv(line);
        const std::size_t comment_pos = sv.find_first_of("#;");
        if (comment_pos != std::string_view::npos) {
            sv = sv.substr(0, comment_pos);
        }
        sv = Trim(sv);
        if (sv.empty() || sv.front() == '[') {
            continue;
        }

        const std::size_t eq_pos = sv.find('=');
        if (eq_pos == std::string_view::npos) {
            continue;
        }

        const std::string key = ToLowerAscii(Trim(sv.substr(0, eq_pos)));
        const std::string_view val = Trim(sv.substr(eq_pos + 1));

        if (key == "enable_thread_affinity_masks") {
            out_config.enable_thread_affinity_masks = ParseBoolValue(val, out_config.enable_thread_affinity_masks);
        } else if (key == "enforce_strict_core_clamps") {
            out_config.enforce_strict_core_clamps = ParseBoolValue(val, out_config.enforce_strict_core_clamps);
        } else if (key == "min_cores_for_isolation_split") {
            out_config.min_cores_for_isolation_split = ParseIntValue(val, out_config.min_cores_for_isolation_split);
        } else if (key == "enable_proportional_scaler") {
            out_config.enable_proportional_scaler = ParseBoolValue(val, out_config.enable_proportional_scaler);
        } else if (key == "core_ratio") {
            ParseCoreRatioValue(val, out_config.core_ratio);
        } else if (key == "maxwell_safe_ubo_passthrough") {
            out_config.maxwell_safe_ubo_passthrough = ParseFeatureOverrideValue(val, out_config.maxwell_safe_ubo_passthrough);
        } else if (key == "intel_qsv_video_offload") {
            out_config.intel_qsv_video_offload = ParseFeatureOverrideValue(val, out_config.intel_qsv_video_offload);
        } else if (key == "vram_shield_prune_trigger_mb") {
            out_config.vram_shield_prune_trigger_mb = ParseIntValue(val, out_config.vram_shield_prune_trigger_mb);
        } else if (key == "vram_shield_ceiling_mb") {
            out_config.vram_shield_ceiling_mb = ParseIntValue(val, out_config.vram_shield_ceiling_mb);
        } else if (key == "vram_shield_lsfg_ceiling_mb") {
            out_config.vram_shield_lsfg_ceiling_mb = ParseIntValue(val, out_config.vram_shield_lsfg_ceiling_mb);
        } else if (key == "bar1_staging_cap_mb") {
            out_config.bar1_staging_cap_mb = ParseIntValue(val, out_config.bar1_staging_cap_mb);
        } else if (key == "audio_priority_tier") {
            out_config.audio_priority_tier = ParsePriorityTierValue(val, out_config.audio_priority_tier);
        } else if (key == "housekeeping_priority_tier") {
            out_config.housekeeping_priority_tier = ParsePriorityTierValue(val, out_config.housekeeping_priority_tier);
        }
    }

    return true;
}

} // namespace ax
