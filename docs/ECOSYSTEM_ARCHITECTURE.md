# The Silicon Preservation Society — Ecosystem Architecture & Repository Topology

> **Canonical Reference:** This document is maintained in [`ax-sdk`](https://github.com/galaxywarriordad/ax-sdk) as the single source of truth for cross-repository architecture across The Silicon Preservation Society ecosystem.

## 1\. Architectural Thesis & Design Principles

The Silicon Preservation Society ecosystem is engineered around **deterministic hardware preservation**: keeping proven x86\_64 and ARM64 computing hardware out of landfills by eliminating OS background jitter, audio resampling, and uncoordinated thread scheduling.

All nine repositories enforce a strict **Policy-in-SDK / Adapter-in-Consumer** boundary across three runtime tiers:

1. **Shared Runtime Core (`ax-sdk`):** Dependency-free C++20 primitives for physical CPU topology discovery, core/IRQ isolation masks, engine configuration parsing, HugeTLB fastmem, VRAM guardrails, and hardware video offloading.  
2. **Bare-Metal OS & Workbench Layer (`Piccolo-AX`, `Piccolo-2go`, `piccolo-ax-desktop`):** Immutable compressed root filesystems (`EROFS` / `SquashFS` running `toram`) with kernel-level CPU partitioning (`isolcpus`, `nohz_full`, `rcu_nocbs`, `irqaffinity`), real-time scheduling (`6.12-rt`), and isolated user-space modes.  
3. **Media & Emulation Consumer Layer (`FLAC_AX`, `Eden-Espresso-Wave-AX`, `Cemu-Latte-AX`, `Amiberry-AX`, `Generation-X-AX`):** Domain-specific C++20 workloads that consume `ax-sdk` policies and OS isolation guarantees without duplicating platform scheduling logic.

```
+-----------------------------------------------------------------------------------+
|                        CONSUMER WORKLOADS & ADAPTERS                              |
|  FLAC_AX  |  Eden-Espresso-Wave-AX  |  Cemu-Latte-AX  |  Amiberry-AX  | Gen-X-AX  |
+-----------------------------------------------------------------------------------+
                                         |
                                 links against (ax::core)
                                         v
+-----------------------------------------------------------------------------------+
|                         SHARED RUNTIME CORE (`ax-sdk`)                            |
|   ax::topology (2:3:1 / 1:2:1)  |  ax::EngineConfig  |  HugeTLB & VRAM Shield     |
+-----------------------------------------------------------------------------------+
                                         |
                             executes on bare metal
                                         v
+-----------------------------------------------------------------------------------+
|                    BARE-METAL OS & WORKBENCH ENVIRONMENTS                         |
|    Piccolo-AX (x86_64 6.12-rt)   |   Piccolo-2go (ARM64 RK3326)   |  Desktop X11  |
+-----------------------------------------------------------------------------------+
```

---

## 2\. Deterministic Core Isolation Topologies

### 6-Core `x86_64` Baseline (`2:3:1` Layout)

Engineered for 6-core desktop processors (such as the Intel Core i5-9600K and i5-8600K):

| Core(s) | Affinity Mask | Scheduling Class | Assigned Workload Domain |
| :---- | :---- | :---- | :---- |
| **Core 0** | `0x01` | `SCHED_FIFO` (Priority 80\) | Bit-perfect USB/ALSA/ASIO audio and low-latency input polling |
| **Core 1** | `0x02` | `SCHED_OTHER` / Isolated | Vulkan / OpenGL / DRM scanout and presentation |
| **Cores 2–4** | `0x1C` | `SCHED_OTHER` / Isolated | Emulator CPU/JIT execution and real-time DSP synthesis |
| **Core 5** | `0x20` | `SCHED_OTHER` (Housekeeping) | Storage I/O, kernel housekeeping, TUI launchers, and batch transcoding (`ax-encode-2go`) |

### 4-Core `aarch64` Handheld Baseline (`1:2:1` Layout)

Engineered for quad-core ARM Cortex-A35 handhelds (`RK3326` — `BATLEXP G350` and `R36S`):

| Core(s) | Affinity Mask | Assigned Workload Domain |
| :---- | :---- | :---- |
| **Core 0** | `0x01` | Audio output (`48000 Hz` ALSA / HiBy FC3 OTG) and input polling |
| **Cores 1–2** | `0x06` | Emulation execution and DRM/KMS \+ Panfrost GLES / `mpv` video decode |
| **Core 3** | `0x08` | Storage I/O, `ax-powerd` thermal/voltage guard, and system housekeeping |

---

## 3\. Repository Directory & Architectural Roles

### Shared Core Layer

#### `ax-sdk`

* **Repository:** [`galaxywarriordad/ax-sdk`](https://github.com/galaxywarriordad/ax-sdk)  
* **Role:** Dependency-free C++20 shared runtime library (`ax_core`).  
* **Architecture:** Exposes `ax::topology` (`Domain`, `SiliconProfile`, `ProbeHostSiliconOnce`, `DetectHostSilicon`, `PinCurrentThread`, and `pin_to_housekeeping_core5()`), `ax::EngineConfig` (`CoreRatioSpec`, `FeatureOverride`, `ThreadPriorityTier`, VRAM shield thresholds), HugeTLB fastmem helpers, and hardware offload flags.  
* **Interface Contract:** Public headers under `include/ax/` maintain strict backward compatibility across all downstream CMake `FetchContent` consumers.

---

### Bare-Metal OS & Workbench Layer

#### `Piccolo-AX`

* **Repository:** [`galaxywarriordad/Piccolo-AX`](https://github.com/galaxywarriordad/Piccolo-AX)  
* **Role:** Bare-metal `x86_64` Buildroot/Batocera Linux (`6.12-rt` real-time kernel) `toram` EROFS console and A/V appliance (`< 300 MB` image footprint).  
* **Architecture:** Enforces the `2:3:1` physical core/IRQ topology at boot via kernel command-line parameters and bootloader mode dispatch (`limine.conf` / `ax-mode-cli`). Includes `/usr/bin/ax-encode-2go`, a Core 5 (`taskset -c 5`) transcoding pipeline pairing Intel QSV (`h264_qsv`, Main profile, zero B-frames, VBV-capped) with NVIDIA CUDA Lanczos downsampling and IPS image tuning to produce `640x480` (`60 fps` / `30 fps`) \+ `48000 Hz` stereo `AAC` / `FLAC` media for `Piccolo-2go`.

#### `Piccolo-2go`

* **Repository:** [`galaxywarriordad/Piccolo-2go`](https://github.com/galaxywarriordad/Piccolo-2go)  
* **Role:** Ultra-lean (`< 140–180 MB` SquashFS) deterministic `aarch64` Buildroot OS for `BATLEXP G350` and `R36S` (`RK3326` Cortex-A35 / Mali-G31 MP2) handhelds.  
* **Architecture:** Enforces `1:2:1` core isolation within a `1024 MB` RAM envelope (`1024 MB` LZ4 ZRAM), custom U-Boot \+ ATF BL31 splash boot, `ax-powerd` thermal/voltage guards, KMS/DRM \+ Panfrost GLES 3.0 rendering, and bit-aligned `48000 Hz` audio routing (internal DAC and HiBy FC3 USB-C OTG). Pairs directly with `ax-encode-2go` outputs for zero-scaling, zero-resampling `mpv` playback (`vo=gpu`, `gpu-context=drm`, `hwdec=no`).

#### `piccolo-ax-desktop`

* **Repository:** [`galaxywarriordad/piccolo-ax-desktop`](https://github.com/galaxywarriordad/piccolo-ax-desktop)  
* **Role:** Isolated on-demand X11 desktop workbench (`< 42 MB` budget).  
* **Architecture:** Provides an opt-in `LXQt`/`XFCE` session (Falkon, AbiWord, Gnumeric, Foliate, 2D/pixel editors, hardware inspection tools, and `mpv`/`yt-dlp`) strictly quarantined from real-time appliance boot modes so desktop daemons and browser threads cannot perturb `Core 0` audio or isolated emulation cores.

---

### Media & Emulation Consumer Layer

#### `FLAC_AX`

* **Repository:** [`galaxywarriordad/FLAC_AX`](https://github.com/galaxywarriordad/FLAC_AX)  
* **Role:** Bare-metal C++20 semi-pro audio player, live MIDI synthesizer workstation, video upscaler, and Core 5 appliance TUI.  
* **Architecture:**  
  * `flac_ax`: Zero-resampling `S32_LE` / `DoP` (`DSD64`/`DSD128`) and `.MOD`/`.SNDH` player locked into RAM (`mlockall`) on `Core 0` (`SCHED_FIFO` 80\) for ESS Sabre DACs (`ES9281PRO` / `ES9281AC Pro`).  
  * `ax_av_studio`: Low-latency MIDI keyboard-mapped synthesizer and OpenGL HUD workstation.  
  * `ax_video_upscaler`: Audio-master synchronized `720p`/`1080p`/`2K` DVD and live retro HDMI upscaler.  
  * `ax_appliance_tui` & `ax_encode_2go`: Links against `ax::topology::pin_to_housekeeping_core5()` to orchestrate appliance workloads and `640x480` G350 transcoding on `Core 5` (`0x20`).

#### `Eden-Espresso-Wave-AX`

* **Repository:** [`galaxywarriordad/Eden-Espresso-Wave-AX`](https://github.com/galaxywarriordad/Eden-Espresso-Wave-AX)  
* **Role:** Hardware-locked fork of the Eden Nintendo Switch emulator.  
* **Architecture:** Built with LLVM `clang-cl`, ThinLTO, and 3-stage Profile-Guided Optimization (PGO), integrating Elastic `2:3:1` core/IRQ partitioning, zero-copy Intel QSV video offloading, Maxwell-safe VRAM guardrails (`5.50 GB` ceiling on 6 GB GPUs), and Steinberg ASIO / ALSA low-latency audio.

#### `Cemu-Latte-AX`

* **Repository:** [`galaxywarriordad/Cemu-Latte-AX`](https://github.com/galaxywarriordad/Cemu-Latte-AX)  
* **Role:** Deterministic Nintendo Wii U emulator fork.  
* **Architecture:** Combines a PGO-optimized Windows baseline (Coffee Lake / Vulkan 1.4) with a Linux `ax-sdk` edition enforcing `2:3:1` core affinity and deterministic frame pacing.

#### `Amiberry-AX`

* **Repository:** [`galaxywarriordad/Amiberry-AX`](https://github.com/galaxywarriordad/Amiberry-AX)  
* **Role:** Deterministic Commodore Amiga (`500` / `1200` / `CD32`, `OCS`/`ECS`/`AGA`) emulator fork.  
* **Architecture:** Integrates HugeTLB fastmem allocation and `2:3:1` thread pinning via `ax-sdk` to eliminate host OS scheduling jitter during cycle-exact chipset emulation.

#### `Generation-X-AX`

* **Repository:** [`galaxywarriordad/Generation-X-AX`](https://github.com/galaxywarriordad/Generation-X-AX)  
* **Role:** Unified C++20 `[AX]` adapter suite for classic consoles and 16-bit microcomputers.  
* **Architecture:** Wraps `RMG` (Nintendo 64), `BlastEm` (Sega Mega Drive), `Cannonball` (Sega OutRun), and `Hatari-AX` (Atari ST) behind consistent `ax::core` topology pinning, HugeTLB fastmem, and lock-free audio/video synchronization adapters.

---

## 4\. Ecosystem Integration & Engineering Standards

* **Single Source of Truth:** Architectural specifications live in `ax-sdk/docs/ECOSYSTEM_ARCHITECTURE.md`; downstream repositories embed a concise topology summary and link directly to this document.  
* **ABI & Header Stability:** Extensions to `ax-sdk` (`include/ax/topology.hpp`, `include/ax/config.hpp`) are strictly additive so downstream `FetchContent` builds remain green across all C++20 consumers.  
* **Automated CI Gates:** Changes to build systems, core isolation scripts, and media pipelines (`test-ax-encode-2go.yml`, `build-release.yml`, `build-sdcard-image.yml`) are verified via automated static analysis (`shellcheck`), stream inspection (`ffprobe`), and headless CMake/Ninja builds.
