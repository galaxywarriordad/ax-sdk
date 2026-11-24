# ⚙️ ax-sdk — Shared C++20 Deterministic Emulation Core

A lightweight, dependency-free C++20 runtime library (`libax_core` / `ax::core`) powering **The Silicon Preservation Society** emulators—engineered to keep proven PC hardware out of landfills.

`ax-sdk` provides a unified hardware-abstraction layer so `[AX]` emulator forks can share deterministic CPU core isolation, HugeTLB/Large-Page fastmem, Intel iGPU video/texture offloading, VRAM protection, and bit-perfect low-latency audio without duplicating platform code.

---

## 🔗 The Silicon Preservation Society — Official 9-Repository Network

*(See [`REPO_TOPOLOGY.md`](http://REPO_TOPOLOGY.md) for the full cross-host dependency diagram and the **Policy-in-SDK / Adapter-in-Consumer** boundary contract.)*

| Project | Role | Canonical Repository | Base / Upstream |
| :---- | :---- | :---- | :---- |
| **`ax-sdk`** | Shared C++20 Runtime Core (`libax_core` / `v1_36_2`) | [GitHub (`ax-sdk`): https\://github.com/galaxywarriordad/ax-sdk](https://github.com/galaxywarriordad/ax-sdk) | Dependency-Free C++20 Core (`MIT`) |
| **Eden Espresso Wave `[AX]`** | Nintendo Switch (Windows 10/11 PGO \+ Linux) | [Forgejo (`Eden-Espresso-Wave-AX`)](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX.git) | Upstream: [Eden](https://git.eden-emu.dev/eden-emu/eden/) (`GPL-3.0`) |
| **Piccolo `[AX]`** | Less than 512 MB Bare-Metal Linux Console OS (`6.12-rt` `toram`) | [GitHub (`Piccolo-AX`)](https://github.com/galaxywarriordad/Piccolo-AX) | OS Base: [Batocera Linux](https://github.com/batocera-linux/batocera.linux) (`GPL-3.0`) |
| **Piccolo-2go** | Deterministic `aarch64` Handheld OS (`RK3326` `1:2:1`) | [GitHub (`Piccolo-2go`)](https://github.com/galaxywarriordad/Piccolo-2go) | Buildroot `aarch64` (`GPL-3.0`) |
| **piccolo-ax-desktop** | Optional Less than 42 MB Tier A X11 Desktop Workbench | [GitHub (`piccolo-ax-desktop`)](https://github.com/galaxywarriordad/piccolo-ax-desktop) | `LXQt`/`XFCE` \+ `SHARE` overlay (`GPL-3.0`) |
| **Cemu Latte `[AX]`** | Nintendo Wii U (Win10/11 PGO \+ Batocera `[AX]`) | [GitHub (`Cemu-Latte-AX`)](https://github.com/galaxywarriordad/Cemu-Latte-AX) | Upstream: [Cemu](https://github.com/cemu-project/Cemu) (`MPL-2.0`) |
| **`FLAC_AX`** | Bare-Metal Linux ESS Sabre Audio & A/V Studio | [GitHub (`FLAC_AX`)](https://github.com/galaxywarriordad/FLAC_AX) | Standalone `libFLAC++` \+ ALSA (`MIT`) |
| **Amiberry `[AX]`** | Commodore Amiga 500 / 1200 / CD32 Engine | [GitHub (`Amiberry-AX`)](https://github.com/galaxywarriordad/Amiberry-AX) | Upstream: [Amiberry](https://github.com/BlitterStudio/amiberry) (`GPL-3.0`) |
| **Generation-X-AX** | Unified Classic Engines `[AX]` Suite | [GitHub (`Generation-X-AX`)](https://github.com/galaxywarriordad/Generation-X-AX) | `RMG`, `BlastEm`, `Cannonball`, `Flycast`, `Hatari` (`GPL-3.0`) |

---

## 🧩 Core Modules & Aggregated Tooling

* **Unified Settings & Platform Macros (`ax::config`):** Standardized platform/ISA macros (`AX_PLATFORM_WINDOWS`, `AX_PLATFORM_LINUX`, `AX_HAS_AVX2`, `AX_HAS_BMI1`) and zero-dependency `ax_engine.ini` loader (`ax::LoadEngineConfig`) shared across all `[AX]` applications.  
* **Lock-Free Atomic Telemetry (`ax::telemetry`):** 64-byte cache-line-isolated atomic counters (`audio_underruns`, `vram_reaps`, `bar1_stalls`, `igpu_offloads`, `qsv_frames_decoded`) exposed via `ax::GetTelemetrySnapshot()` for OSDs and external daemons.  
* **Deterministic Topology (`ax::topology`):** Enforces `2:3:1` Non-SMT, dynamic Intel SMT / AMD CCX physical core affinity masks, and `RK3326` `1:2:1` masks alongside real-time thread priorities (`SCHED_FIFO` / MMCSS).  
* **Zero-Overhead Memory (`ax::memory`):** `1G`/`2M` HugeTLB and Windows Large-Page fastmem allocators with single-cycle `BMI1` (`_bextr_u64`) page translation.  
* **Intel iGPU Offloading (`ax::igpu`):** Offloads hardware video decoding (`QSV` / `VA-API` / `VEBOX`) and compute texture unswizzling onto Intel UHD Graphics.  
* **VRAM & BAR1 Safeguards (`ax::vram`):** Proactive `5.00 GiB` async pruning, per-GPU Vulkan VRAM ceilings (`3.50 GiB` to `10.00 GiB` via `config/hardware_profiles.json`), bounded `128 MiB` / `256 MiB` `HOST_VISIBLE` BAR1 staging pools, and runtime `FP16` to `FP32` shader promotion.  
* **Bit-Perfect Audio Ring (`ax::audio`):** Lock-free, cache-line-aligned PCM ring buffer targeting `2.00 ms` ALSA / `2.67 ms` ASIO exclusive USB DAC playback.  
* **Aggregated Host Tooling (`tools/`):** Process-parameterized scripts (`Get-AxProfile.ps1`, `check_hardware.ps1`, `set_irq_affinity.ps1`, `track_telemetry.ps1`, `check_regression.ps1`, `clean_all.ps1`, and Linux `ax_doctor.sh`) for pre-flight audits, IRQ steering, and telemetry collection across `[AX]` targets.

---

## 📂 Repository Structure

```
ax-sdk/
├── .github/workflows/ (Automated CI validation & multi-platform build checks: ci.yml)
├── ax_toolchain/ (Cross-platform compiler & toolchain presets)
├── config/ (Declarative silicon profile matrix: hardware_profiles.json)
├── include/ax/ (Public C++20 headers: config, telemetry, topology, memory, igpu, vram, audio)
├── src/ (Zero-dependency C++20 implementation & OS shims)
├── tools/ (Aggregated cross-app host scripts: Get-AxProfile, check_hardware, set_irq_affinity, track_telemetry, check_regression, clean_all, ax_doctor.sh)
├── REPO_TOPOLOGY.md (9-repository lineage & Policy-in-SDK boundary contract)
├── CI_Maturity.md (CMM Levels 1-5 quality & Hardware-in-the-Loop roadmap)
└── CMakeLists.txt (CMake target: libax_core / ax::core, v1.36.2)
```

---

## 🚀 CMake `FetchContent` Integration (`v1_36_2`)

```
include(FetchContent)
FetchContent_Declare(
  ax_sdk
  GIT_REPOSITORY https://github.com/galaxywarriordad/ax-sdk.git
  GIT_TAG v1_36_2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ax_sdk)
target_link_libraries(your_target PRIVATE ax::core)
```

---

## 🗺️ What's Next — `ax-sdk` Runtime Roadmap & `Espresso` Proof Points

### Active Release: `ax-sdk` `v1.36.2` & `Eden Espresso Wave [AX]`

`ax-sdk` (`v1.36.2`) powers **Eden Espresso Wave `[AX]`** on reference desktop hardware (`Intel Core i5-9600K` \+ `NVIDIA GTX 980 Ti 6 GB` \+ `Intel UHD Graphics 630`) with automated Linux and Windows (`clang-cl`) CI checks alongside JSON schema validation.

* **Deterministic `2:3:1` Core & IRQ Partitioning (`ax::topology`):** Pins real-time USB DAC audio/timing (`0x01`), Vulkan command submission (`0x02`), ARM64 JIT worker fibers (`0x1C`), and background ZSTD/FFmpeg decompression (`0x20`) to match steered PCIe/USB/AHCI interrupts, eliminating cross-core thread migration and audio dropouts.  
* **Maxwell `GM200` VRAM & PCIe BAR1 Protection (`ax::vram`):** Guards 6 GB Maxwell/Pascal cards against driver-level shared-memory thrashing via runtime `FP16` to `FP32` shader promotion and a bounded **`128 MiB` `HOST_VISIBLE` BAR1** staging ceiling.  
* **Low-Latency Audio Ring (`ax::audio`):** Drives exclusive USB DAC playback via cache-line-isolated SPSC stereo ring buffers (`LockFreeStereoRing`).

---

### Upcoming `ax-sdk` Milestones (`v1.37+`)

#### `v1.37` — Lifecycle API, CI Hardware Mocks & iGPU `ASTC` to `BC7` Transcoding

* **Explicit Context & Lifecycle API (`ax::Init` / `ax::Shutdown`):** Deterministic initialization returning structured `ax::Status` diagnostic codes (`Status::Ok`, `Status::FallbackNoHugePages`, `Status::FallbackSingleGpu`) to simplify multi-emulator onboarding.  
* **Hardware-Mock Unit & Concurrency CI (`tests/`):** GitHub Actions (`clang-cl` on Windows, `Clang`/`GCC` on Linux) running `CTest` suites for SPSC ring-buffer memory ordering, `2:3:1` \+ L3 CCX bitmask math across simulated 4C/6C/8C/16C topologies, and VRAM threshold hysteresis.  
* **Intel UHD 630 FP16 `ASTC` to `BC7` Compute Offload (`ax::igpu`):** Offloads async `ASTC` to `BC7` block transcoding onto idle Intel UHD 630 EUs (`24 EUs`, native `2:1` FP16) via `dma_buf` / NT shared handles, cutting resident texture VRAM by **4:1** (`8 bpp` vs. `32 bpp` `RGBA8`) for native Maxwell BC7 hardware sampling.  
* **Batched Guest SMC Page-Protection Updates (`ax::memory`):** Batches `VirtualProtect` / `mprotect` invalidations (`FEX-Emu` pattern) to eliminate 4 KB page-fault storms during JIT self-modifying code flushes.

---

## Silicon Preservation Society — All 9 Repositories

* [The Silicon Preservation Society (`[AX]`) vs. Raspberry Pi — Architectural Comparison & Roadmap](http://docs/PI_VS_AX_ARCHITECTURE.md)  
1. **Shared Silicon SDK (GitHub):** [`galaxywarriordad/ax-sdk`](https://github.com/galaxywarriordad/ax-sdk.git) — Shared dependency-free C++20 deterministic hardware runtime (`ax::core` / `v1_36_2`), dynamic Intel SMT / AMD CCX / `RK3326` `1:2:1` topology resolver, platform shims, and master architecture specs.  
2. **Linux USB Console & A/V Appliance (GitHub):** [`galaxywarriordad/Piccolo-AX`](https://github.com/galaxywarriordad/Piccolo-AX.git) — Standalone `< 512 MB` `toram` EROFS Batocera / Buildroot `Linux 6.12-rt` `x86_64` appliance (`batocera-ax-external`) with direct ALSA `SCHED_FIFO` and `iHD` VA-API `dma_buf`.  
3. **Linux Handheld Appliance (GitHub):** [`galaxywarriordad/Piccolo-2go`](https://github.com/galaxywarriordad/Piccolo-2go.git) — Deterministic Buildroot `aarch64` (`RK3326` `1:2:1`) retro handheld OS for **BATLEXP G350** and **R36S**.  
4. **Optional Desktop Tier (GitHub):** [`galaxywarriordad/piccolo-ax-desktop`](https://github.com/galaxywarriordad/piccolo-ax-desktop.git) — On-demand `< 42 MB` Tier A LXQt/X11 desktop workbench isolated on housekeeping cores.  
5. **Bare-Metal Audio & A/V Engine (GitHub):** [`galaxywarriordad/FLAC_AX`](https://github.com/galaxywarriordad/FLAC_AX.git) — RAM-resident bit-perfect audio player, `ax_av_studio` MIDI/DSP synth workstation, `ax_video_upscaler` stream pipeline, and `ax_appliance_tui`.  
6. **Commodore Amiga Engine (GitHub):** [`galaxywarriordad/Amiberry-AX`](https://github.com/galaxywarriordad/Amiberry-AX.git) — Deterministic Commodore Amiga 500 / 1200 / CD32 engine linked against `ax-sdk` (`v1_36_2`).  
7. **Retro Multi-System Engine (GitHub):** [`galaxywarriordad/Generation-X-AX`](https://github.com/galaxywarriordad/Generation-X-AX.git) — Unified C++20 `[AX]` adapter suite powering `RMG`, `BlastEm`, `Cannonball`, `Frontier`, `Flycast`, `Hatari`, and `EDuke32`.  
8. **Nintendo Switch Engine Fork (Forgejo):** [`galaxywarriordad/Eden-Espresso-Wave-AX`](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX.git) — Deterministic Nintendo Switch fork tuned for `ax::core` core topology, Vulkan telemetry, LLVM `clang-cl` PGO, and low-latency audio.  
9. **Nintendo Wii U Engine Fork (GitHub):** [`galaxywarriordad/Cemu-Latte-AX`](https://github.com/galaxywarriordad/Cemu-Latte-AX.git) — Deterministic Nintendo Wii U (Latte GPU) `[AX]` engine powered by `ax::core` (`v1_36_2`).

For full core-pinning tables (`2:3:1` and `1:2:1`), cross-repository data flow, and architectural specifications, see [`REPO_TOPOLOGY.md`](http://REPO_TOPOLOGY.md) and [`docs/ECOSYSTEM_ARCHITECTURE.md`](http://docs/ECOSYSTEM_ARCHITECTURE.md).

---

## 🤝 Contributing & License

* **Contributing:** See [CONTRIBUTING.md](http://CONTRIBUTING.md) and [SECURITY.md](http://SECURITY.md) for compiler hygiene (`clang-cl` 23+ `-Werror`) and hardware-privilege notes.  
* **License:** Licensed under the **MIT License** ([LICENSE](http://LICENSE)) for seamless integration across GPL-3.0, MPL-2.0, and MIT codebases.
