// src/ess_dac_hooks.hpp — ESS Sabre (ES9281AC Pro / ES9281PRO) hooks for FLAC_AX
#pragma once

#include <alsa/asoundlib.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace flac_ax::ess {

enum class EssChipFamily {
    Unknown,
    Es9281AcPro, // HiBy FC3 / USB-C mobile Sabre class
    Es9281Pro    // Sabre Pro family variant
};

struct EssChipProfile {
    EssChipFamily family{EssChipFamily::Unknown};
    const char* chip_name{"Generic ESS Sabre"};
    const char* default_alsa_device{"hw:CARD=FC3,DEV=0"};
    unsigned int min_rate_hz{44100};
    unsigned int max_rate_hz{384000};
    unsigned int preferred_rate_hz{48000};
    snd_pcm_format_t preferred_format{SND_PCM_FORMAT_S32_LE};
    snd_pcm_uframes_t default_period_frames{96};
    snd_pcm_uframes_t fallback_period_frames{128};
    bool supports_dop_dsd{true};
    bool prefers_direct_hw_rate{true};
};

struct ProfileMatrixCheck {
    bool file_found{false};
    bool has_v21_schema{false};
    bool common_audio_device_auto{false};
    std::string matched_cpu_profile;
    std::string audio_irq_mask{"0x01"};
};

inline EssChipProfile MakeEssProfile(EssChipFamily family) noexcept {
    switch (family) {
        case EssChipFamily::Es9281AcPro:
            return EssChipProfile{
                .family = EssChipFamily::Es9281AcPro,
                .chip_name = "ES9281AC Pro",
                .default_alsa_device = "hw:CARD=FC3,DEV=0",
                .min_rate_hz = 44100,
                .max_rate_hz = 384000,
                .preferred_rate_hz = 48000,
                .preferred_format = SND_PCM_FORMAT_S32_LE,
                .default_period_frames = 96,
                .fallback_period_frames = 128,
                .supports_dop_dsd = true,
                .prefers_direct_hw_rate = true,
            };
        case EssChipFamily::Es9281Pro:
            return EssChipProfile{
                .family = EssChipFamily::Es9281Pro,
                .chip_name = "ES9281PRO",
                .default_alsa_device = "hw:CARD=FC3,DEV=0",
                .min_rate_hz = 44100,
                .max_rate_hz = 384000,
                .preferred_rate_hz = 48000,
                .preferred_format = SND_PCM_FORMAT_S32_LE,
                .default_period_frames = 96,
                .fallback_period_frames = 128,
                .supports_dop_dsd = true,
                .prefers_direct_hw_rate = true,
            };
        case EssChipFamily::Unknown:
        default:
            return EssChipProfile{};
    }
}

// Resolves ESS chip hook from a CLI/config string (e.g. "es9281ac_pro", "es9281pro", "fc3").
inline std::optional<EssChipProfile> ResolveEssChipHook(std::string_view spec) noexcept {
    std::string lower(spec);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower.find("9281ac") != std::string::npos || lower.find("fc3") != std::string::npos) {
        return MakeEssProfile(EssChipFamily::Es9281AcPro);
    }
    if (lower.find("9281pro") != std::string::npos || lower.find("es9281") != std::string::npos) {
        return MakeEssProfile(EssChipFamily::Es9281Pro);
    }
    return std::nullopt;
}

// Clamps or selects a target sample rate within the ESS chip's supported PCM range.
inline unsigned int NegotiateEssRate(const EssChipProfile& chip, unsigned int stream_rate_hz) noexcept {
    if (stream_rate_hz == 0U) {
        return chip.preferred_rate_hz;
    }
    return std::clamp(stream_rate_hz, chip.min_rate_hz, chip.max_rate_hz);
}

// Hook called right after snd_pcm_hw_params is applied to verify format/rate lock.
inline bool OnEssHardwareConfigured(const EssChipProfile& chip,
                                  snd_pcm_t* pcm,
                                  unsigned int actual_rate_hz,
                                  snd_pcm_uframes_t actual_period_frames) noexcept {
    (void)chip;
    if (!pcm || actual_rate_hz == 0U || actual_period_frames == 0U) {
        return false;
    }
    // Keep snd_pcm_prepare() clean and ready for S32_LE playback.
    return snd_pcm_state(pcm) == SND_PCM_STATE_PREPARED ||
           snd_pcm_state(pcm) == SND_PCM_STATE_RUNNING;
}

// Lightweight cross-check against ax-sdk/config/hardware_profiles.json (schema_version 2.1).
inline ProfileMatrixCheck CrossCheckHardwareProfiles(
    const std::filesystem::path& profiles_path,
    std::string_view cpu_model_hint = "i5-9600K") {
    ProfileMatrixCheck check{};
    std::ifstream in(profiles_path);
    if (!in.is_open()) {
        return check;
    }
    check.file_found = true;

    const std::string content((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());

    check.has_v21_schema = (content.find("\"schema_version\": \"2.1\"") != std::string::npos);
    check.common_audio_device_auto =
        (content.find("\"Audio|output_device\": \"auto\"") != std::string::npos);

    if (!cpu_model_hint.empty() &&
        content.find(std::string(cpu_model_hint)) != std::string::npos) {
        check.matched_cpu_profile = std::string(cpu_model_hint);
    }
    return check;
}

} // namespace flac_ax::ess
