# **🗺️ End-to-End Ecosystem & Architectural Overview**

**Programme:** The Silicon Preservation Society (`[AX]`)  
**Mission:** Keep proven PC hardware out of landfills through deterministic bare-metal optimization.  
**SDK & Engine Baseline:** `ax-sdk` `v1_36_2` (`v1.36.2`) \| `Eden-Espresso-Wave-AX` `v1.35.1`  
**Last Updated:** `2026-11-24`

The `[AX]` ecosystem transforms commodity `x86_64` and `aarch64` hardware into a deterministic, console-grade media and gaming platform. It enforces strict spatial core partitioning (2:3:1 and 1:2:1), unified configuration (ax\_engine.ini), zero-copy cross-silicon offloading, and shared-memory atomic telemetry across four architectural layers and nine repositories.

# **1\. 🏗️ 4-Layer Ecosystem & Silicon Architecture**

* **Layer 1: Application Cartridges** — Includes *Eden Espresso Wave `[AX]`* (Switch), *Cemu Latte `[AX]`* (Wii U), *Amiberry `[AX]`* (Commodore Amiga 500/1200/CD32), *Generation-X-AX* (N64, Mega Drive, OutRun, Frontier: Elite II, Dreamcast, Atari ST, EDuke32), and `FLAC_AX` (Hi-Res Audio, A/V Studio & Video Upscaler) hosted across Forgejo and GitHub under `galaxywarriordad/` repositories (`GPL-3.0`, `MPL-2.0`, `MIT`). Combines two-tier release disciplines (Stage A control baselines vs. Stage B production builds with `ax::core` isolation) and a low-latency `FLAC_AX` pipeline (`2.00 ms` ALSA).  
* **Layer 2: Shared Core, Settings & Tooling (`ax-sdk`)** — Features a C++20 runtime (`topology`, `memory`, `vram`, `igpu`, `audio`, `config`, shared-memory `telemetry` with audio jitter profiler), declarative truth profiles (`hardware_profiles.json`), and aggregated PowerShell/Bash administration scripts (`tools/`).  
* **Layer 3: Bare-Metal Console OS & Workbench (`Piccolo-AX`, `Piccolo-2go` & `piccolo-ax-desktop`)** — Includes `< 512 MB` RAM-resident EROFS (`lz4hc,9`) `Piccolo-AX` built on pinned `batocera.linux` and `Linux 6.12-rt` (`PREEMPT_RT`, `NO_HZ_FULL`, `cgroup v2`, ZRAM LZ4, and A/B `.prev` rollback), `< 140 MB` `zstd` SquashFS `Piccolo-2go` (`aarch64` `RK3326` `1:2:1`), and the optional `< 42 MB` Tier A `piccolo-ax-desktop` LXQt/X11 workbench isolated on Core `5`.  
* **Layer 4: Stream 1 Reference Silicon** — Built around the Intel Core i5-9600K (`6C/6T`, zero SMT) isolated across Core 0–4 for audio/timing, Vulkan/gather, and guest JIT, leaving Core 5 for kernel `kworker`s, storage/audio IRQs, and decoding, paired with a GTX 980 Ti, UHD 630, and HiBy FC3 DAC.

# **2\. 🔄 Unified Data-Flow, Settings & Telemetry Pipeline**

Data flows from declarative configuration files (`hardware_profiles.json` and `ax_engine.ini`) into the shared C++20 runtime (ax::core), dividing workloads across pinned CPU cores: Core 0 handles audio/timing and USB/ALSA/FlexASIO interrupts; Core 1 handles Vulkan submission and VRAM management; Cores 2–4 execute guest JIT fibers or real-time synth DSP; and Core 5 handles QSV/iHD VA-API decoding, MIDI polling, and storage tasks. Metrics are written to cross-process shared-memory atomic regions (`Local\AX_Telemetry` on Windows or `/dev/shm/ax_telemetry` on Linux) to feed in-engine ImGui/Vulkan OSDs and monitoring scripts.

# **3\. 🧩 Detailed Layer Breakdown**

* **Layer 1 (Cartridges):** Details shader promotion (`GM200` `FP16 -> FP32`), BAR1 staging caps, and audio ring buffers for Eden, Cemu, Amiberry, Generation-X-AX, and `FLAC_AX`.  
* **Layer 2 (`ax-sdk` `v1_36_2`):** Core C++20 modules cover configuration parsing (`config.hpp/cpp`), shared-memory telemetry and jitter stats (`telemetry.hpp/cpp`), P-core/SMT and `1:2:1` topology discovery (`topology.hpp/cpp`), huge-page memory allocators (`memory.hpp/cpp`), VRAM pruning (`vram.hpp`), iGPU-to-dGPU handoffs (`igpu.hpp`), and lock-free stereo audio rings (`audio.hpp`). Tooling scripts automate profile resolution, hardware checks, IRQ affinity, live telemetry tracking, and regression testing.  
* **Layer 3 (`Piccolo-AX`, `Piccolo-2go` & `piccolo-ax-desktop`):** Out-of-tree Buildroot overlays (`batocera-ax-external/`) building real-time kernels, boot isolation scripts (`S01piccolo-isolate`, `S02piccolo-hw`, `K01piccolo-sync`), rootfs image constraints, and Core 5 desktop isolation.  
* **Layer 4 (Reference Silicon):** Specific hardware specifications for the i5-9600K (`4.8 GHz` locked), GTX 980 Ti / UHD 630 GPU pair, and HiBy FC3 USB DAC.

# **4\. 📂 9-Repository File Tree Summary**

```
[AX] Ecosystem/
├── ax-sdk/                    # [MIT] Shared C++20 Core, Settings & Cross-App Tools (v1_36_2)
│   ├── config/hardware_profiles.json  # Declarative silicon profile matrix (schema_version: 2.1)
│   ├── include/ax/            # config.hpp, telemetry.hpp, topology.hpp, memory.hpp, vram.hpp, igpu.hpp, audio.hpp
│   ├── src/                   # config.cpp, telemetry.cpp, topology.cpp, memory.cpp
│   ├── tools/                 # Get-AxProfile, check_hardware, set_irq_affinity, track_telemetry, check_regression, clean_all, ax_doctor.sh
│   ├── ax_toolchain/          # piccolo-clang-x86_64.cmake
│   └── REPO_TOPOLOGY.md       # 9-repo network & Policy-in-SDK boundary contract
│
├── Eden-Espresso-Wave-AX/     # [GPL-3.0] Windows 10/11 Switch Engine Master (v1.35.1)
│   ├── PGO_config.bat         # Interactive configurator & 3-stage LLVM PGO orchestrator
│   ├── Scripts/               # resolve_ax_tools.bat, macro_update.ps1, shader_validate.ps1, tail_eden_log.ps1, reset_eden_config.ps1
│   └── src/                   # Switch engine + Vulkan 1.4 VkVramShield + FlexASIO/QSV adapters
│
├── Piccolo-AX/                # [GPL-3.0] < 512 MB toram Batocera Linux Console OS
│   └── batocera-ax-external/  # Buildroot BR2_EXTERNAL (6.12-rt kernel, package/*.mk, rootfs-overlay/)
│
├── Piccolo-2go/               # [GPL-3.0] < 140 MB aarch64 RK3326 1:2:1 Handheld OS
│   └── board/piccolo-2go/     # Buildroot external tree for BATLEXP G350 & R36S
│
├── piccolo-ax-desktop/        # [GPL-3.0] Optional < 42 MB Tier A LXQt/X11 Workbench
│   └── scripts/               # Core 5 isolated desktop & SHARE persistence launchers
│
├── Cemu-Latte-AX/             # [MPL-2.0] Wii U Emulator Fork (Win10 PGO + Piccolo-AX)
│   └── src/                   # Cemu 2.x + ax::core integration
│
├── Amiberry-AX/               # [GPL-3.0] Commodore Amiga 500 / 1200 / CD32 Engine
│   └── src/                   # Amiberry + ax::core integration
│
├── Generation-X-AX/           # [GPL-3.0] Unified Classic Engines [AX] Suite
│   └── engines/               # RMG, BlastEm, Cannonball, Frontier, Flycast, Hatari, EDuke32
│
└── FLAC_AX/                   # [MIT] Bare-Metal Linux ESS Sabre Audio & A/V Studio
    └── src/main.cpp           # Core 5 FLAC/DoP decoder -> ax::audio::LockFreeStereoRing -> Core 0 SCHED_FIFO ALSA
```

Full repository layouts, integration scripts, and source files can be referenced directly in source documents.
