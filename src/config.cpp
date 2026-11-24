#include "ax/config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>

namespace ax {
namespace {

std::string Trim(std::string_view sv) {
  while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
    sv.remove_prefix(1);
  }
  while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
    sv.remove_suffix(1);
  }
  return std::string(sv);
}

std::string ToLower(std::string_view sv) {
  std::string out(sv);
  for (char& ch : out) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return out;
}

bool ParseBool(std::string_view raw, bool& out) noexcept {
  const std::string v = ToLower(Trim(raw));
  if (v == "1" || v == "true" || v == "yes" || v == "on") {
    out = true;
    return true;
  }
  if (v == "0" || v == "false" || v == "no" || v == "off") {
    out = false;
    return true;
  }
  return false;
}

bool ParseInt(std::string_view raw, int& out) noexcept {
  const std::string v = Trim(raw);
  if (v.empty()) {
    return false;
  }
  char* end = nullptr;
  const long val = std::strtol(v.c_str(), &end, 10);
  if (end == v.c_str() || *end != '\0') {
    return false;
  }
  out = static_cast<int>(val);
  return true;
}

FeatureOverride ParseFeatureOverride(std::string_view raw) noexcept {
  const std::string v = ToLower(Trim(raw));
  if (v == "1" || v == "true" || v == "enabled" || v == "enable" || v == "on" || v == "force_on") {
    return FeatureOverride::Enabled;
  }
  if (v == "0" || v == "false" || v == "disabled" || v == "disable" || v == "off" || v == "force_off") {
    return FeatureOverride::Disabled;
  }
  return FeatureOverride::Auto;
}

ThreadPriorityTier ParsePriorityTier(std::string_view raw) noexcept {
  const std::string v = ToLower(Trim(raw));
  if (v == "3" || v == "housekeeping" || v == "idle" || v == "low") {
    return ThreadPriorityTier::Housekeeping;
  }
  if (v == "2" || v == "realtime_audio" || v == "realtimeaudio" || v == "critical" || v == "high") {
    return ThreadPriorityTier::RealtimeAudio;
  }
  if (v == "1" || v == "elevated" || v == "above_normal") {
    return ThreadPriorityTier::Elevated;
  }
  return ThreadPriorityTier::Normal;
}

} // namespace

bool LoadEngineConfig(std::string_view path, EngineConfig& out_config) noexcept {
  try {
    std::ifstream file{std::string(path)};
    if (!file.is_open()) {
      return false;
    }

    std::string line;
    while (std::getline(file, line)) {
      const std::size_t hash_pos = line.find('#');
      if (hash_pos != std::string::npos) {
        line.erase(hash_pos);
      }
      const std::size_t semi_pos = line.find(';');
      if (semi_pos != std::string::npos) {
        line.erase(semi_pos);
      }
      const std::string trimmed = Trim(line);
      if (trimmed.empty() || trimmed.front() == '[') {
        continue;
      }

      const std::size_t eq_pos = trimmed.find('=');
      if (eq_pos == std::string::npos) {
        continue;
      }

      const std::string key = ToLower(Trim(std::string_view(trimmed).substr(0, eq_pos)));
      const std::string val = Trim(std::string_view(trimmed).substr(eq_pos + 1));

      bool b = false;
      int n = 0;

      if (key == "enable_thread_affinity_masks" && ParseBool(val, b)) {
        out_config.enable_thread_affinity_masks = b;
      } else if (key == "enforce_strict_core_clamps" && ParseBool(val, b)) {
        out_config.enforce_strict_core_clamps = b;
      } else if (key == "min_cores_for_isolation_split" && ParseInt(val, n)) {
        out_config.min_cores_for_isolation_split = n;
      } else if (key == "enable_proportional_scaler" && ParseBool(val, b)) {
        out_config.enable_proportional_scaler = b;
      } else if (key == "core_ratio_audio_gpu" && ParseInt(val, n)) {
        out_config.core_ratio.audio_gpu = n;
      } else if (key == "core_ratio_guest_jit" && ParseInt(val, n)) {
        out_config.core_ratio.guest_jit = n;
      } else if (key == "core_ratio_aux_quarantine" && ParseInt(val, n)) {
        out_config.core_ratio.aux_quarantine = n;
      } else if (key == "maxwell_safe_ubo_passthrough") {
        out_config.maxwell_safe_ubo_passthrough = ParseFeatureOverride(val);
      } else if (key == "intel_qsv_video_offload") {
        out_config.intel_qsv_video_offload = ParseFeatureOverride(val);
      } else if (key == "vram_shield_prune_trigger_mb" && ParseInt(val, n)) {
        out_config.vram_shield_prune_trigger_mb = n;
      } else if (key == "vram_shield_ceiling_mb" && ParseInt(val, n)) {
        out_config.vram_shield_ceiling_mb = n;
      } else if (key == "vram_shield_lsfg_ceiling_mb" && ParseInt(val, n)) {
        out_config.vram_shield_lsfg_ceiling_mb = n;
      } else if (key == "bar1_staging_cap_mb" && ParseInt(val, n)) {
        out_config.bar1_staging_cap_mb = n;
      } else if (key == "audio_priority_tier") {
        out_config.audio_priority_tier = ParsePriorityTier(val);
      } else if (key == "housekeeping_priority_tier") {
        out_config.housekeeping_priority_tier = ParsePriorityTier(val);
      }
    }
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace ax
