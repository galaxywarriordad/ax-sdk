// src/main.cpp — Bare-metal Linux FLAC -> ALSA prototype with ESS DAC & RAM preload hooks
#include "ess_dac_hooks.hpp"
#include "preload_policy.hpp"

#include <FLAC++/decoder.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if defined(FLAC_AX_HAS_AX_SDK)
#if __has_include(<ax/audio.hpp>)
#include <ax/audio.hpp>
#define FLAC_AX_HAVE_AX_AUDIO_HPP 1
#endif
#endif

namespace {

std::atomic<bool> g_stop_requested{false};

void HandleSignal(int /*sig*/) noexcept {
    g_stop_requested.store(true, std::memory_order_release);
}

template <std::size_t CapacityFrames>
class StereoFrameRing {
public:
#if defined(FLAC_AX_HAVE_AX_AUDIO_HPP)
    bool Push(const std::int32_t* interleaved_lr, std::size_t frames) noexcept {
        return ax_ring_.PushBatch(interleaved_lr, frames);
    }

    std::size_t Pop(std::int32_t* out_interleaved_lr, std::size_t max_frames) noexcept {
        return ax_ring_.PopBatch(out_interleaved_lr, max_frames);
    }

    std::size_t Size() const noexcept {
        return ax_ring_.Size();
    }
#else
    bool Push(const std::int32_t* interleaved_lr, std::size_t frames) noexcept {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        const std::size_t used = (head + CapacityFrames - tail) % CapacityFrames;
        const std::size_t free_slots = (CapacityFrames - 1) - used;
        if (frames > free_slots) {
            return false;
        }

        const std::size_t first = std::min(frames, CapacityFrames - head);
        std::memcpy(&buffer_[head][0], interleaved_lr, first * 2 * sizeof(std::int32_t));
        const std::size_t second = frames - first;
        if (second > 0) {
            std::memcpy(&buffer_[0][0], interleaved_lr + first * 2, second * 2 * sizeof(std::int32_t));
        }

        head_.store((head + frames) % CapacityFrames, std::memory_order_release);
        return true;
    }

    std::size_t Pop(std::int32_t* out_interleaved_lr, std::size_t max_frames) noexcept {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t available = (head + CapacityFrames - tail) % CapacityFrames;
        const std::size_t to_pop = std::min(max_frames, available);
        if (to_pop == 0) {
            return 0;
        }

        const std::size_t first = std::min(to_pop, CapacityFrames - tail);
        std::memcpy(out_interleaved_lr, &buffer_[tail][0], first * 2 * sizeof(std::int32_t));
        const std::size_t second = to_pop - first;
        if (second > 0) {
            std::memcpy(out_interleaved_lr + first * 2, &buffer_[0][0], second * 2 * sizeof(std::int32_t));
        }

        tail_.store((tail + to_pop) % CapacityFrames, std::memory_order_release);
        return to_pop;
    }

    std::size_t Size() const noexcept {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        return (head + CapacityFrames - tail) % CapacityFrames;
    }
#endif

private:
#if defined(FLAC_AX_HAVE_AX_AUDIO_HPP)
    ax::audio::LockFreeStereoRing<CapacityFrames, std::int32_t> ax_ring_{};
#else
    alignas(64) std::array<std::array<std::int32_t, 2>, CapacityFrames> buffer_{};
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
#endif
};

struct StreamMeta {
    std::atomic<unsigned int> sample_rate{0};
    std::atomic<unsigned int> bits_per_sample{0};
    std::atomic<unsigned int> channels{0};
    std::atomic<bool> meta_ready{false};
    std::atomic<bool> eof_or_error{false};
};

inline std::int32_t SampleToS32(FLAC__int32 sample, unsigned int bps) noexcept {
    if (bps >= 32U) {
        return static_cast<std::int32_t>(sample);
    }
    if (bps == 0U) {
        return 0;
    }
    const unsigned int shift = 32U - bps;
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(sample) << shift);
}

class FlacStreamDecoder final : public FLAC::Decoder::File {
public:
    FlacStreamDecoder(StereoFrameRing<65536>& ring, StreamMeta& meta)
        : ring_(ring), meta_(meta) {}

protected:
    void on_metadata(const ::FLAC__StreamMetadata* metadata) override {
        if (metadata->type == ::FLAC__METADATA_TYPE_STREAMINFO) {
            meta_.sample_rate.store(metadata->data.stream_info.sample_rate, std::memory_order_release);
            meta_.bits_per_sample.store(metadata->data.stream_info.bits_per_sample, std::memory_order_release);
            meta_.channels.store(metadata->data.stream_info.channels, std::memory_order_release);
            meta_.meta_ready.store(true, std::memory_order_release);
        }
    }

    ::FLAC__StreamDecoderWriteStatus write_callback(
        const ::FLAC__Frame* frame,
        const FLAC__int32* const buffer[]) override {
        const std::size_t blocksize = frame->header.blocksize;
        const unsigned int bps = frame->header.bits_per_sample;
        const unsigned int channels = frame->header.channels;

        if (pcm_scratch_.size() < blocksize * 2) {
            pcm_scratch_.resize(blocksize * 2);
        }

        for (std::size_t i = 0; i < blocksize; ++i) {
            const std::int32_t l = SampleToS32(buffer[0][i], bps);
            const std::int32_t r = (channels >= 2U) ? SampleToS32(buffer[1][i], bps) : l;
            pcm_scratch_[i * 2 + 0] = l;
            pcm_scratch_[i * 2 + 1] = r;
        }

        std::size_t offset = 0;
        while (offset < blocksize && !g_stop_requested.load(std::memory_order_acquire)) {
            if (ring_.Push(pcm_scratch_.data() + offset * 2, blocksize - offset)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        return g_stop_requested.load(std::memory_order_acquire)
                   ? FLAC__STREAM_DECODER_WRITE_STATUS_ABORT
                   : FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    void error_callback(::FLAC__StreamDecoderErrorStatus status) override {
        std::cerr << "[FLAC] Decoder error: " << ::FLAC__StreamDecoderErrorStatusString[status] << "\n";
        meta_.eof_or_error.store(true, std::memory_order_release);
    }

private:
    StereoFrameRing<65536>& ring_;
    StreamMeta& meta_;
    std::vector<std::int32_t> pcm_scratch_;
};

void ApplyThreadAffinityAndPriority(int cpu_id, int sched_fifo_prio) noexcept {
    if (cpu_id >= 0) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(static_cast<unsigned int>(cpu_id), &set);
        if (pthread_setaffinity_np(pthread_self(), sizeof(set), &set) != 0) {
            std::cerr << "[WARN] Failed to set CPU affinity to core " << cpu_id << "\n";
        }
    }

    if (sched_fifo_prio > 0) {
        sched_param param{};
        param.sched_priority = sched_fifo_prio;
        if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
            std::cerr << "[WARN] Failed to set SCHED_FIFO priority " << sched_fifo_prio
                      << " (check CAP_SYS_NICE / ulimit)\n";
        }
    }
}

bool ConfigureAlsaForEss(snd_pcm_t* pcm,
                         const flac_ax::ess::EssChipProfile& chip,
                         unsigned int stream_rate_hz,
                         snd_pcm_uframes_t period_frames,
                         unsigned int& out_actual_rate,
                         snd_pcm_uframes_t& out_actual_period) {
    snd_pcm_hw_params_t* params = nullptr;
    snd_pcm_hw_params_alloca(&params);

    int err = snd_pcm_hw_params_any(pcm, params);
    if (err < 0) {
        std::cerr << "[ALSA] snd_pcm_hw_params_any failed: " << snd_strerror(err) << "\n";
        return false;
    }

    err = snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        std::cerr << "[ALSA] snd_pcm_hw_params_set_access failed: " << snd_strerror(err) << "\n";
        return false;
    }

    err = snd_pcm_hw_params_set_format(pcm, params, chip.preferred_format);
    if (err < 0) {
        std::cerr << "[ALSA] snd_pcm_hw_params_set_format(S32_LE) failed: " << snd_strerror(err) << "\n";
        return false;
    }

    err = snd_pcm_hw_params_set_channels(pcm, params, 2);
    if (err < 0) {
        std::cerr << "[ALSA] snd_pcm_hw_params_set_channels(2) failed: " << snd_strerror(err) << "\n";
        return false;
    }

    const unsigned int negotiated = flac_ax::ess::NegotiateEssRate(chip, stream_rate_hz);
    unsigned int rate = negotiated;
    int dir = 0;
    err = snd_pcm_hw_params_set_rate_near(pcm, params, &rate, &dir);
    if (err < 0) {
        std::cerr << "[ALSA] snd_pcm_hw_params_set_rate_near failed: " << snd_strerror(err) << "\n";
        return false;
    }

    snd_pcm_uframes_t period = period_frames;
    err = snd_pcm_hw_params_set_period_size_near(pcm, params, &period, &dir);
    if (err < 0) {
        std::cerr << "[ALSA] snd_pcm_hw_params_set_period_size_near failed: " << snd_strerror(err) << "\n";
        return false;
    }

    snd_pcm_uframes_t buffer = period * 4;
    err = snd_pcm_hw_params_set_buffer_size_near(pcm, params, &buffer);
    if (err < 0) {
        std::cerr << "[ALSA] snd_pcm_hw_params_set_buffer_size_near failed: " << snd_strerror(err) << "\n";
        return false;
    }

    err = snd_pcm_hw_params(pcm, params);
    if (err < 0) {
        std::cerr << "[ALSA] snd_pcm_hw_params failed: " << snd_strerror(err) << "\n";
        return false;
    }

    err = snd_pcm_prepare(pcm);
    if (err < 0) {
        std::cerr << "[ALSA] snd_pcm_prepare failed: " << snd_strerror(err) << "\n";
        return false;
    }

    out_actual_rate = rate;
    out_actual_period = period;

    if (!flac_ax::ess::OnEssHardwareConfigured(chip, pcm, out_actual_rate, out_actual_period)) {
        std::cerr << "[ESS] Hardware state verification failed for " << chip.chip_name << "\n";
        return false;
    }

    std::cout << "[ESS] Chip=" << chip.chip_name
              << " rate=" << out_actual_rate << " Hz (stream=" << stream_rate_hz << " Hz)"
              << ", period=" << out_actual_period
              << " frames, buffer=" << buffer << " frames\n";
    return true;
}

std::filesystem::path ResolveHardwareProfilesPath(std::string_view cli_path) {
    if (!cli_path.empty()) {
        return std::filesystem::path(cli_path);
    }
    if (const char* env_dir = std::getenv("AX_SDK_DIR"); env_dir && *env_dir) {
        return std::filesystem::path(env_dir) / "config" / "hardware_profiles.json";
    }
    return std::filesystem::path("../ax-sdk/config/hardware_profiles.json");
}

void PrintUsage() {
    std::cerr << "Usage: flac_ax <file.flac> [options]\n"
              << "  --ess <es9281ac_pro|es9281pro>   Target ESS DAC chip hook (default: es9281ac_pro)\n"
              << "  --device <alsa_dev>              ALSA PCM device (default: chip default)\n"
              << "  --period <frames>                ALSA period frames (default: chip default 96)\n"
              << "  --profiles <path/to/json>        Cross-check ax-sdk hardware_profiles.json\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    std::string flac_path;
    std::string ess_arg = "es9281ac_pro";
    std::string device_override;
    std::string profiles_arg;
    bool has_period_override = false;
    snd_pcm_uframes_t period_frames = 96U;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--ess" && i + 1 < argc) {
            ess_arg = argv[++i];
        } else if (arg == "--device" && i + 1 < argc) {
            device_override = argv[++i];
        } else if (arg == "--period" && i + 1 < argc) {
            period_frames = static_cast<snd_pcm_uframes_t>(std::stoul(argv[++i]));
            has_period_override = true;
        } else if (arg == "--profiles" && i + 1 < argc) {
            profiles_arg = argv[++i];
        } else if (!arg.empty() && arg.front() != '-' && flac_path.empty()) {
            flac_path = std::string(arg);
        } else {
            PrintUsage();
            return 1;
        }
    }

    if (flac_path.empty()) {
        PrintUsage();
        return 1;
    }

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    const auto resolved_ess = flac_ax::ess::ResolveEssChipHook(ess_arg);
    if (!resolved_ess.has_value()) {
        std::cerr << "[ESS] Unrecognized ESS chip spec '" << ess_arg
                  << "'. Use 'es9281ac_pro' or 'es9281pro'.\n";
        return 1;
    }
    const flac_ax::ess::EssChipProfile chip = *resolved_ess;
    const std::string device =
        !device_override.empty() ? device_override : std::string(chip.default_alsa_device);
    if (!has_period_override) {
        period_frames = chip.default_period_frames;
    }

    const auto profiles_path = ResolveHardwareProfilesPath(profiles_arg);
    const auto matrix = flac_ax::ess::CrossCheckHardwareProfiles(profiles_path);
    if (matrix.file_found) {
        std::cout << "[AX-SDK] Cross-checked " << profiles_path.string()
                  << " (v2.1=" << (matrix.has_v21_schema ? "yes" : "no")
                  << ", output_device=auto=" << (matrix.common_audio_device_auto ? "yes" : "no")
                  << ", cpu_match=" << (matrix.matched_cpu_profile.empty() ? "none" : matrix.matched_cpu_profile)
                  << ")\n";
    }

    const auto preload_policy = flac_ax::DetectHostPreloadPolicy();
    std::cout << "[PRELOAD] MemTotal=" << (preload_policy.detected_mem_total_bytes / (1024ULL * 1024ULL))
              << " MiB, mode="
              << (preload_policy.mode == flac_ax::PreloadMode::WholeAlbum ? "WholeAlbum" :
                  (preload_policy.mode == flac_ax::PreloadMode::Window3Files ? "Window3Files" : "SingleFile"))
              << "\n";

    StereoFrameRing<65536> ring;
    StreamMeta meta;
    std::atomic<bool> decode_finished{false};

    std::thread decoder_thread([&]() {
        ApplyThreadAffinityAndPriority(5, 0);

        FlacStreamDecoder decoder(ring, meta);
        const FLAC__StreamDecoderInitStatus init_status = decoder.init(flac_path);
        if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
            std::cerr << "[FLAC] Failed to initialize decoder for '" << flac_path << "' (status="
                      << FLAC__StreamDecoderInitStatusString[init_status] << ")\n";
            meta.eof_or_error.store(true, std::memory_order_release);
            decode_finished.store(true, std::memory_order_release);
            return;
        }

        decoder.process_until_end_of_stream();
        decoder.finish();
        decode_finished.store(true, std::memory_order_release);
    });

    while (!meta.meta_ready.load(std::memory_order_acquire) &&
           !meta.eof_or_error.load(std::memory_order_acquire) &&
           !g_stop_requested.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (meta.eof_or_error.load(std::memory_order_acquire) ||
        g_stop_requested.load(std::memory_order_acquire)) {
        g_stop_requested.store(true, std::memory_order_release);
        decoder_thread.join();
        return 1;
    }

    const unsigned int stream_rate = meta.sample_rate.load(std::memory_order_acquire);
    const unsigned int bps = meta.bits_per_sample.load(std::memory_order_acquire);
    const unsigned int channels = meta.channels.load(std::memory_order_acquire);
    std::cout << "[FLAC] Stream: " << stream_rate << " Hz, " << bps << "-bit, " << channels << " ch\n";

    snd_pcm_t* pcm = nullptr;
    int open_err = snd_pcm_open(&pcm, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (open_err < 0) {
        std::cerr << "[ALSA] Failed to open '" << device << "': " << snd_strerror(open_err) << "\n";
        g_stop_requested.store(true, std::memory_order_release);
        decoder_thread.join();
        return 1;
    }

    unsigned int actual_rate = 0;
    snd_pcm_uframes_t actual_period = 0;
    if (!ConfigureAlsaForEss(pcm, chip, stream_rate, period_frames, actual_rate, actual_period)) {
        snd_pcm_close(pcm);
        g_stop_requested.store(true, std::memory_order_release);
        decoder_thread.join();
        return 1;
    }

    ApplyThreadAffinityAndPriority(0, 80);

    std::vector<std::int32_t> out_buffer(actual_period * 2, 0);
    std::uint64_t xruns = 0;
    std::uint64_t total_frames_written = 0;

    while (!g_stop_requested.load(std::memory_order_acquire)) {
        const std::size_t avail = ring.Size();
        if (avail == 0) {
            if (decode_finished.load(std::memory_order_acquire)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(250));
            continue;
        }

        const std::size_t to_write = std::min<std::size_t>(avail, static_cast<std::size_t>(actual_period));
        const std::size_t popped = ring.Pop(out_buffer.data(), to_write);
        if (popped == 0) {
            continue;
        }

        std::size_t frame_offset = 0;
        while (frame_offset < popped && !g_stop_requested.load(std::memory_order_acquire)) {
            const snd_pcm_sframes_t written =
                snd_pcm_writei(pcm, out_buffer.data() + frame_offset * 2, popped - frame_offset);
            if (written >= 0) {
                frame_offset += static_cast<std::size_t>(written);
                total_frames_written += static_cast<std::size_t>(written);
            } else if (written == -EAGAIN) {
                snd_pcm_wait(pcm, 10);
            } else if (written == -EPIPE) {
                ++xruns;
                const int rec_err = snd_pcm_recover(pcm, static_cast<int>(written), 1);
                if (rec_err < 0) {
                    std::cerr << "[ALSA] Failed to recover from EPIPE: " << snd_strerror(rec_err) << "\n";
                    g_stop_requested.store(true, std::memory_order_release);
                    break;
                }
            } else {
                std::cerr << "[ALSA] snd_pcm_writei error: " << snd_strerror(static_cast<int>(written)) << "\n";
                g_stop_requested.store(true, std::memory_order_release);
                break;
            }
        }
    }

    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);

    g_stop_requested.store(true, std::memory_order_release);
    decoder_thread.join();

    std::cout << "[DONE] Chip=" << chip.chip_name
              << ", frames=" << total_frames_written
              << ", rate=" << actual_rate << " Hz, xruns=" << xruns << "\n";
    return 0;
}
