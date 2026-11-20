markdown
# 🗺️ End-to-End Ecosystem & Architectural Overview

**Programme:** The Silicon Preservation Society (`[AX]`)  
**Mission:** Keep proven PC hardware out of landfills through deterministic bare-metal optimization.  
**SDK & Engine Baseline:** `ax-sdk` `v1.35.1c` / `v1.36.0` | `Eden-Espresso-Wave-AX` `v1.35.1`  
**Last Updated:** `2026-11-20`

The `[AX]` ecosystem transforms commodity `x86_64` desktop hardware into a deterministic, console-grade media and gaming platform. Rather than treating emulation and high-resolution audio as generic desktop processes subject to OS scheduler drift, the stack enforces strict spatial core partitioning (`2:3:1`), unified configuration (`ax_engine.ini`), zero-copy cross-silicon offloading, and lock-free atomic telemetry across **four architectural layers** and **five repositories**.

---

## 1. 🏗️ 4-Layer Ecosystem & Silicon Architecture

| Layer | Component / Target | Repository & License | Architectural Role & Execution Specifications |
| :--- | :--- | :--- | :--- |
| **Layer 1: Application Cartridges** | • **Eden Espresso Wave `[AX]`** (Switch)<br>• **Cemu Latte `[AX]`** (Wii U)<br>• **`FLAC_AX`** (Hi-Res Audio) | • [Eden Forgejo](https://eden-emu.dev) / [GitHub](https://github.com) (`GPL-3.0`)<br>• [Cemu-Latte-AX](https://github.com) (`MPL-2.0`)<br>• [FLAC_AX](https://github.com) (`MIT`) | **Two-Tier Release Discipline (Emulators):**<br>• **Stage A (Control Baseline):** Unmodified upstream compiled with `Clang 23.1+` ThinLTO + 3-Phase PGO (`-march=skylake`) & Vulkan 1.4.<br>• **Stage B (`[AX]` Production):** Links `ax::core`, loads `ax_engine.ini`, and activates full silicon isolation.<br>**Audio Appliance (`FLAC_AX`):** Direct `libFLAC++` → `S32_LE` → `SCHED_FIFO 80` ALSA (`2.00 ms`) pipeline. |
| **Layer 2: Shared Core, Settings & Tooling** | **`ax-sdk`** (`libax_core` / `ax::core`) + **`ax-sdk/tools/`** | [://github.com](https://://github.com) (`MIT`) | • **C++20 Runtime:** `topology`, `memory`, `vram`, `igpu`, `audio`, `config` (`ax_engine.ini`), and 64B-aligned atomic `telemetry`.<br>• **Declarative Truth:** `config/hardware_profiles.json` (`schema_version: 2.1`).<br>• **Aggregated Scripts (`tools/`):** Process-agnostic `check_hardware.ps1`, `set_irq_affinity.ps1`, `telemetry_daemon.ps1`, and `clean_all.ps1`. |
| **Layer 3: Bare-Metal Console OS** | **Piccolo `[AX]`** (`BR2_EXTERNAL`) | [://github.com](https://://github.com) (`GPL-3.0`) | `< 300 MB` RAM-resident EROFS (`lz4hc,9`) appliance built on pinned `batocera.linux` + `Linux 6.12-rt` (`PREEMPT_RT`, `NO_HZ_FULL`, `cgroup v2`, ZRAM LZ4, uncomposited Xorg, A/B `.prev` rollback). |
| **Layer 4: Stream 1 Reference Silicon** | **Intel Core i5-9600K** (`6C/6T`, Zero SMT)<br>**GTX 980 Ti** + **UHD 630** + **HiBy FC3** | Bare-Metal Reference Hardware | • **Isolated Domain (`Cores 0–4`):** Core 0 (`0x01`) Audio/Timing, Core 1 (`0x02`) Vulkan + Dirty Gather, Cores 2–4 (`0x1C`) Guest JIT.<br>• **Housekeeping Sink (`Core 5` / `0x20`):** Kernel `kworker`s, `rcu_nocbs`, SATA/NVMe IRQs, QSV/VA-API, ZSTD L18, and FLAC decoding.<br>• **GPUs & DAC:** GTX 980 Ti (`580.*` / `582.78`), UHD 630 (`iHD` / QSV `NV12`), HiBy FC3 ESS ES9281AC Pro (`hw:CARD=FC3,DEV=0`). |

---

## 2. 🔄 Unified Data-Flow, Settings & Telemetry Pipeline

```text
  [ CONFIGURATION & SILICON TRUTH ]                [ SHARED C++20 RUNTIME (ax::core) ]
  ax-sdk/config/hardware_profiles.json (v2.1) ───► ax::topology (2:3:1 P-Core / SMT / CCX Scaler)
  ax_engine.ini (Unified Runtime Settings)   ───► ax::LoadEngineConfig() ──► Eden / Cemu / FLAC_AX
                                                         │
         ┌───────────────────────────────────────────────┼───────────────────────────────────────────────┐
         ▼                                               ▼                                               ▼
  [ CORE 0 (0x01) — AUDIO/TIMING ]             [ CORE 1 (0x02) — GPU SUBMIT ]               [ CORE 5 (0x20) — AUX SINK ]
  • ax::audio::LockFreeStereoRing              • VulkanWorker + Dirty Gather                • QSV / iHD VA-API NV12 Decode
  • FlexASIO (2.67ms) / ALSA (2.00ms)          • 5.00 GiB Async VRAM Shield                 • Async VRAM Reaper & SPIR-V
  • HiBy FC3 USB xHCI IRQ (0x01)               • 128 MiB BAR1 Cap + GTX 980 Ti IRQ          • FLAC Decoder / ZSTD / Storage IRQs
         │                                               │                                               │
         └───────────────────────────────────────────────┼───────────────────────────────────────────────┘
                                                         ▼
                                    [ CACHE-LINE-ISOLATED ATOMIC TELEMETRY ]
                                    ax::GetTelemetry() (alignas(64) Counters)
                                         │                              │
                                         ▼                              ▼
                          In-Engine Vulkan / ImGui OSD     ax-sdk/tools/telemetry_daemon.ps1
                                                           (-TargetProcess eden | Cemu | flac_ax)
```

---

## 3. 🧩 Detailed Layer Breakdown

### Layer 1: Application Cartridges (`Eden`, `Cemu`, `FLAC_AX`)
* **Eden Espresso Wave `[AX]` (`v1.35.1`):** Windows 10/11 desktop development master and primary Switch engine. Implements Maxwell `GM200` `FP16 → FP32` shader promotion, `128 MiB` `HOST_VISIBLE` BAR1 staging caps, HLE `LibraryApplet` defaults, and `Lossless.dll` frame-generation interop.
* **Cemu Latte `[AX]`:** Dual-edition Wii U emulator combining a Windows 10/11 `Clang 23.1+` PGO baseline with full `ax::core` isolation (`0x1C` PowerPC JIT, `VEBOX` video offload, `5.50 GiB` VRAM shield) on `Piccolo-AX`.
* **`FLAC_AX`:** Minimalist bare-metal Linux audio appliance (`libFLAC++` → `S32_LE` stereo) demonstrating the `[AX]` audio pipeline in isolation: non-RT FLAC decoding on `Core 5` (`0x20`) feeding `ax::audio::LockFreeStereoRing` (`65,536` frames) into a `SCHED_FIFO 80` ALSA output thread on `Core 0` (`0x01`, `hw:CARD=FC3,DEV=0`, `96` frames / `2.00 ms`).

### Layer 2: Shared C++20 SDK, Settings & Aggregated Tooling (`ax-sdk`)
* **Core C++20 Modules (`include/ax/` & `src/`):**
  * `config.hpp` / `config.cpp`: Standardized platform/ISA macros (`AX_PLATFORM_WINDOWS`, `AX_PLATFORM_LINUX`, `AX_HAS_AVX2`, `AX_HAS_BMI1`) and zero-dependency `ax_engine.ini` parser (`ax::LoadEngineConfig`) governing `CoreRatio=2:3:1`, VRAM/BAR1 ceilings, audio buffer sizes, telemetry intervals, and priority clamps.
  * `telemetry.hpp`: 64-byte cache-line-padded atomic counters (`ax::TelemetryCounters`) separated across Audio (`Core 0`), GPU/VRAM (`Core 1`/`5`), and Frame Pacing domains to eliminate false sharing, exposed via lock-free `ax::GetTelemetrySnapshot()`.
  * `topology.hpp` / `topology.cpp`: Cross-platform physical P-core enumerator (`GetLogicalProcessorInformationEx` `LTP_PC_SMT` + `EfficiencyClass` on Windows; `/sys/devices/system/cpu` `thread_siblings_list` + `cpu_capacity` on Linux) with RAII MMCSS `Pro Audio` cleanup and `/dev/cpu/0/msr` (`0xc8f`) Intel CAT detection.
  * `memory.hpp` / `memory.cpp`: `1G` / `2M` HugeTLB and Windows `SeLockMemoryPrivilege` Large-Page fastmem allocators, single-cycle BMI1 `_bextr_u64` page extraction, and `_MM_HINT_NTA` non-temporal streaming hints.
  * `vram.hpp`: Proactive `5.00 GiB` (`kPruneTriggerBytes`) async VRAM pruning (`~4.50 GiB` hysteresis recovery) ahead of `5.50 GiB` / `5.35 GiB` (`lsfg-vk`) / `5.20 GiB` (Win32 LS) hard ceilings via `VK_QUEUE_TRANSFER_BIT` and Vulkan 1.4 split barriers.
  * `igpu.hpp`: Zero-copy `DRM_FORMAT_MOD_LINEAR` `NV12` `dma_buf` descriptors and `<= 3.50 GB/s` DDR4 ring-bus bandwidth guardrails for Intel UHD 630 → GTX 980 Ti handoffs.
  * `audio.hpp`: 64-byte aligned templated `LockFreeStereoRing` (`PushBatch`, `PopBatch`, `PopPlanarBatch`) supporting both `96`-frame (`2.00 ms`) ALSA and `128`-sample (`2.67 ms`) ASIO loops.
* **Aggregated Cross-App Tooling (`ax-sdk/tools/`):**
  * Consolidates host-level auditing (`check_hardware.ps1` for C-states, USB Selective Suspend, `SeLockMemoryPrivilege`, and PCIe/USB/SATA IRQ masks), IRQ steering (`set_irq_affinity.ps1`, `irq_runtime_helper.ps1`), workspace cleanup (`clean_all.ps1`), and out-of-process telemetry collection (`telemetry_daemon.ps1`) so all `[AX]` applications share a single operational harness.

### Layer 3: Bare-Metal Linux Appliance (`Piccolo-AX` / `BR2_EXTERNAL`)
* **Out-of-Tree Buildroot Overlay (`batocera-ax-external/`):** Builds `libax_core`, `eden-ax`, `cemu-latte-ax` (and optionally `flac-ax`) with `Clang 23.1+` ThinLTO on top of a pinned `batocera.linux` base.
* **Real-Time Kernel & Boot Isolation:** Configures `Linux 6.12-rt` (`CONFIG_PREEMPT_RT=y`, `isolcpus=managed_irq,domain,0-4`, `nohz_full=0-4`, `rcu_nocbs=0-4`, `6 × 1G + 1024 × 2M` HugePages, `mitigations=off`), strips `~300 MB` of desktop/compute bloat, and enforces a `<= 300 MiB` (`307,200 KB`) EROFS `lz4hc,9` `toram` image budget with A/B `.prev` rollback.
* **Init & Hardware Gate Scripts:** `S01piccolo-isolate` (`cgroup v2` isolated partition + Core 5 workqueue routing), `S02piccolo-hw` (`nrpacks=1`, Core 0 xHCI & Core 1 GPU IRQ pinning), `K01piccolo-sync` (atomic `zstd -3` shader cache flush on exit), and `verify_ax_appliance.sh` (7-point boot gate).

### Layer 4: Target Stream 1 Reference Silicon
* **CPU:** Intel Core i5-9600K (`6C/6T` Non-SMT, locked symmetrically at `4.8 GHz` / `x48`, Uncore Ring `x45`, `100%` min processor state / C-states locked).

  * dGPU + iGPU: NVIDIA GeForce GTX 980 Ti (6 GB GDDR5, Maxwell GM200, pinned to 582.78 WHQL / 580.* Linux legacy) paired with active on-die Intel UHD Graphics 630 (0x8086, 31.0.101.2135 / iHD VA-API).
  * USB DAC: HiBy FC3 (ESS Sabre ES9281AC Pro, 24-bit / 48 kHz exclusive, isolated on its own xHCI root hub with USB Selective Suspend disabled).

4. 📂 5-Repository File Tree Summary
