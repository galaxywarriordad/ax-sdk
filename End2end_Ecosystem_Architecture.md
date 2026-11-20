# 🗺️ End-to-End Ecosystem & Architectural Overview

**Programme:** The Silicon Preservation Society (`[AX]`)  
**Mission:** Keep proven PC hardware out of landfills through deterministic bare-metal optimization.  
**SDK & Engine Baseline:** `ax-sdk` `v1_36_1` (`v1.36.1`) | `Eden-Espresso-Wave-AX` `v1.35.1`  
**Last Updated:** `2026-11-20`

The `[AX]` ecosystem transforms commodity `x86_64` desktop hardware into a deterministic, console-grade media and gaming platform. It enforces strict spatial core partitioning (`2:3:1`), unified configuration (`ax_engine.ini`), zero-copy cross-silicon offloading, and shared-memory atomic telemetry across four architectural layers and five repositories `[7]`.

---

## 1. 🏗️ 4-Layer Ecosystem & Silicon Architecture

* **Layer 1: Application Cartridges** — Includes *Eden Espresso Wave `[AX]`* (Switch), *Cemu Latte `[AX]`* (Wii U), and *`FLAC_AX`* (Hi-Res Audio) hosted across Forgejo and GitHub under `galaxywarriordad/` repositories (`GPL-3.0`, `MPL-2.0`, `MIT`) `[7]`. Combines two-tier release disciplines (Stage A control baselines vs. Stage B production builds with `ax::core` isolation) and a low-latency `FLAC_AX` pipeline (`2.00 ms` ALSA) `[7]`.
* **Layer 2: Shared Core, Settings & Tooling (`ax-sdk`)** — Features a C++20 runtime (`topology`, `memory`, `vram`, `igpu`, `audio`, `config`, shared-memory `telemetry` with audio jitter profiler), declarative truth profiles (`hardware_profiles.json`), and aggregated PowerShell/Bash administration scripts (`tools/`) `[7]`.
* **Layer 3: Bare-Metal Console OS (`Piccolo AX`)** — A `< 300 MB` RAM-resident EROFS (`lz4hc,9`) appliance built on pinned `batocera.linux` and `Linux 6.12-rt` (`PREEMPT_RT`, `NO_HZ_FULL`, `cgroup v2`, ZRAM LZ4, and A/B `.prev` rollback) `[7]`.
* **Layer 4: Stream 1 Reference Silicon** — Built around the Intel Core i5-9600K (`6C/6T`, zero SMT) isolated across Core 0–4 for audio/timing, Vulkan/gather, and guest JIT, leaving Core 5 for kernel `kworker`s, storage/audio IRQs, and decoding, paired with a GTX 980 Ti, UHD 630, and HiBy FC3 DAC `[7]`.

---

## 2. 🔄 Unified Data-Flow, Settings & Telemetry Pipeline

Data flows from declarative configuration files (`hardware_profiles.json` and `ax_engine.ini`) into the shared C++20 runtime (`ax::core`), dividing workloads across pinned CPU cores: Core 0 handles audio/timing and USB/ALSA/FlexASIO interrupts; Core 1 handles Vulkan submission and VRAM management; and Core 5 handles QSV/iHD VA-API decoding and storage tasks `[7]`. Metrics are written to cross-process shared-memory atomic regions (`Local\AX_Telemetry` on Windows or `/dev/shm/ax_telemetry` on Linux) to feed in-engine ImGui OSDs and monitoring scripts `[7]`.

---

## 3. 🧩 Detailed Layer Breakdown

* **Layer 1 (Cartridges):** Details shader promotion (`GM200` `FP16 → FP32`), BAR1 staging caps, and audio ring buffers for Eden, Cemu, and `FLAC_AX` `[7]`.
* **Layer 2 (`ax-sdk` `v1_36_1`):** Core C++20 modules cover configuration parsing (`config.hpp/cpp`), shared-memory telemetry and jitter stats (`telemetry.hpp/cpp`), P-core/SMT topology discovery (`topology.hpp/cpp`), huge-page memory allocators (`memory.hpp/cpp`), VRAM pruning (`vram.hpp`), iGPU-to-dGPU handoffs (`igpu.hpp`), and lock-free stereo audio rings (`audio.hpp`) `[7]`. Tooling scripts automate profile resolution, hardware checks, IRQ affinity, live telemetry tracking, and regression testing `[7]`.
* **Layer 3 (`Piccolo-AX`):** Out-of-tree Buildroot overlays (`batocera-ax-external/`) building real-time kernels, boot isolation scripts (`S01piccolo-isolate`, `S02piccolo-hw`), and rootfs image constraints `[7]`.
* **Layer 4 (Reference Silicon):** Specific hardware specifications for the i5-9600K (`4.8 GHz` locked), GTX 980 Ti / UHD 630 GPU pair, and HiBy FC3 USB DAC `[7]`.

---

## 4. 📂 5-Repository File Tree Summary

```text
[AX] Ecosystem/
├── ax-sdk/                                 # [MIT] Shared C++20 Core, Settings & Cross-App Tools (v1_36_1)
│   ├── config/hardware_profiles.json       # Declarative silicon profile matrix (schema_version: 2.1)
│   ├── include/ax/                         # config.hpp, telemetry.hpp, topology.hpp, memory.hpp, vram.hpp, igpu.hpp, audio.hpp
│   ├── src/                                # config.cpp, telemetry.cpp, topology.cpp, memory.cpp
│   ├── tools/                              # Get-AxProfile, check_hardware, set_irq_affinity, track_telemetry, check_regression, clean_all, ax_doctor.sh
│   ├── ax_toolchain/                       # piccolo-clang-x86_64.cmake
│   └── REPO_TOPOLOGY.md                    # 5-repo network & Policy-in-SDK boundary contract
│
├── Eden-Espresso-Wave-AX/                  # [GPL-3.0] Windows 10/11 Switch Engine Master (v1.35.1)
│   ├── PGO_config.bat                      # Interactive configurator & 3-stage LLVM PGO orchestrator
│   ├── Scripts/                            # resolve_ax_tools.bat, macro_update.ps1, shader_validate.ps1, tail_eden_log.ps1, reset_eden_config.ps1
│   └── src/                                # Switch engine + Vulkan 1.4 VkVramShield + FlexASIO/QSV adapters
│
├── Piccolo-AX/                             # [GPL-3.0] < 300 MB toram Batocera Linux Console OS
│   └── batocera-ax-external/               # Buildroot BR2_EXTERNAL (6.12-rt kernel, package/*.mk, rootfs-overlay/)
│
├── Cemu-Latte-AX/                          # [MPL-2.0] Wii U Emulator Fork (Win10 PGO + Piccolo-AX)
│   └── src/                                # Cemu 2.x + ax::core integration
│
└── FLAC_AX/                                # [MIT] Bare-Metal Linux ESS Sabre Audio Appliance
    └── src/main.cpp                        # Core 5 FLAC decoder -> ax::audio::LockFreeStereoRing -> Core 0 SCHED_FIFO ALSA
```
Full repository layouts, integration scripts, and source files can be referenced directly in source document `[7]`.
