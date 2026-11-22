// include/ax/config.hpp — Unified platform macros & zero-dependency ax_engine.ini configuration loader
#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>

#if defined(_WIN32) || defined(_WIN64)
#ifndef AX_PLATFORM_WINDOWS
#define AX_PLATFORM_WINDOWS 1
#endif
#elif defined(__linux__)
#ifndef AX_PLATFORM_LINUX
#define AX_PLATFORM_LINUX 1
#endif
#endif

#if defined(__AVX2__)
#define AX_HAS_AVX2 1
#else
#define AX_HAS_AVX2 0
#endif

#if defined(__BMI__)
#define AX_HAS_BMI1 1
#else
#define AX_HAS_BMI1 0
#endif

namespace ax {

struct EngineConfig {
    bool enable_shared_memory_export{true};
    bool enable_huge_pages{true};
    bool enable_igpu_offload{true};
    bool enable_fp16_promotion{true};
    std::uint32_t audio_period_frames{96U};
    std::uint32_t audio_sample_rate_hz{48000U};
    std::uint64_t vram_ceiling_bytes{5905580032ULL};     // 5.50 GiB
    std::uint64_t vram_reap_trigger_bytes{5368709120ULL}; // 5.00 GiB
    std::uint64_t bar1_staging_limit_bytes{134217728ULL}; // 128 MiB
    std::string telemetry_shm_name{"/ax_telemetry"};
};

namespace detail {

inline std::string_view TrimWs(std::string_view sv) noexcept {
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r' || sv.front() == '\n')) {
        sv.remove_prefix(1U);
    }
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r' || sv.back() == '\n')) {
        sv.remove_suffix(1U);
    }
    return sv;
}

inline bool ParseBoolValue(std::string_view v, bool fallback) noexcept {
    if (v == "1" || v == "true" || v == "TRUE" || v == "yes" || v == "on") return true;
    if (v == "0" || v == "false" || v == "FALSE" || v == "no" || v == "off") return false;
    return fallback;
}

inline std::uint64_t ParseU64Value(std::string_view v, std::uint64_t fallback) noexcept {
    std::uint64_t parsed = 0ULL;
    const auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), parsed);
    return (ec == std::errc{} && ptr == v.data() + v.size()) ? parsed : fallback;
}

} // namespace detail

inline bool LoadEngineConfig(const std::string& ini_path, EngineConfig& out_cfg) {
    std::ifstream file(ini_path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string_view sv = detail::TrimWs(line);
        if (sv.empty() || sv.front() == '#' || sv.front() == ';' || sv.front() == '[') {
            continue;
        }
        const std::size_t eq = sv.find('=');
        if (eq == std::string_view::npos) {
            continue;
        }
        const std::string_view key = detail::TrimWs(sv.substr(0U, eq));
        const std::string_view val = detail::TrimWs(sv.substr(eq + 1U));

        if (key == "enable_shared_memory_export") {
            out_cfg.enable_shared_memory_export = detail::ParseBoolValue(val, out_cfg.enable_shared_memory_export);
        } else if (key == "enable_huge_pages") {
            out_cfg.enable_huge_pages = detail::ParseBoolValue(val, out_cfg.enable_huge_pages);
        } else if (key == "enable_igpu_offload") {
            out_cfg.enable_igpu_offload = detail::ParseBoolValue(val, out_cfg.enable_igpu_offload);
        } else if (key == "enable_fp16_promotion") {
            out_cfg.enable_fp16_promotion = detail::ParseBoolValue(val, out_cfg.enable_fp16_promotion);
        } else if (key == "audio_period_frames") {
            out_cfg.audio_period_frames = static_cast<std::uint32_t>(
                detail::ParseU64Value(val, out_cfg.audio_period_frames));
        } else if (key == "audio_sample_rate_hz") {
            out_cfg.audio_sample_rate_hz = static_cast<std::uint32_t>(
                detail::ParseU64Value(val, out_cfg.audio_sample_rate_hz));
        } else if (key == "vram_ceiling_bytes") {
            out_cfg.vram_ceiling_bytes = detail::ParseU64Value(val, out_cfg.vram_ceiling_bytes);
        } else if (key == "vram_reap_trigger_bytes") {
            out_cfg.vram_reap_trigger_bytes = detail::ParseU64Value(val, out_cfg.vram_reap_trigger_bytes);
        } else if (key == "bar1_staging_limit_bytes") {
            out_cfg.bar1_staging_limit_bytes = detail::ParseU64Value(val, out_cfg.bar1_staging_limit_bytes);
        } else if (key == "telemetry_shm_name" && !val.empty()) {
            out_cfg.telemetry_shm_name.assign(val.data(), val.size());
        }
    }
    return true;
}

} // namespace ax
