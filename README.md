# ⚙️ ax-sdk — Shared C++20 Deterministic Emulation Core

A lightweight, dependency-free C++20 runtime library (`libax_core` / `ax::core`) powering **The Silicon Preservation Society** emulators—engineered to keep proven PC hardware out of landfills.

`ax-sdk` provides a unified hardware-abstraction layer so `[AX]` emulator forks can share deterministic CPU core isolation, HugeTLB/Large-Page fastmem, Intel iGPU video/texture offloading, VRAM protection, and bit-perfect low-latency audio without duplicating platform code.

---

## 🔗 The Silicon Preservation Society — Official 9-Repository Network

*(See [`REPO_TOPOLOGY.md`](REPO_TOPOLOGY.md) for the full cross-host dependency diagram and the **Policy-in-SDK / Adapter-in-Consumer** boundary contract.)*

| Project | Role | Canonical Repository | Base / Upstream |
| :--- | :--- | :--- | :--- |
| **`ax-sdk`** | Shared C++20 Runtime Core (`libax_core` / `v1_36_2`) | [GitHub (`ax-sdk`)](https://github.com/galaxywarriordad/ax-sdk) | Dependency-Free C++20 Core (`MIT`) |
| **Eden Espresso Wave `[AX]`** | Nintendo Switch (Windows 10/11 PGO + Linux) | [Forgejo (`Eden-Espresso-Wave-AX`)](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX.git) | Upstream: [Eden](https://git.eden-emu.dev/eden-emu/eden/) (`GPL-3.0`) |
| **Piccolo `[AX]`** | `< 512 MB` Bare-Metal Linux Console OS (`6.12-rt` `toram`) | [GitHub (`Piccolo-AX`)](https://github.com/galaxywarriordad/Piccolo-AX) | OS Base: [Batocera Linux](https://github.com/batocera-linux/batocera.linux) (`GPL-3.0`) |
| **Piccolo-2go** | Deterministic `aarch64` Handheld OS (`RK3326` `1:2:1`) | [GitHub (`Piccolo-2go`)](https://github.com/galaxywarriordad/Piccolo-2go) | Buildroot `aarch64` (`GPL-3.0`) |
| **piccolo-ax-desktop** | Optional `< 42 MB` Tier A X11 Desktop Workbench | [GitHub (`piccolo-ax-desktop`)](https://github.com/galaxywarriordad/piccolo-ax-desktop) | `LXQt`/`XFCE` + `SHARE` overlay (`GPL-3.0`) |
| **Cemu Latte `[AX]`** | Nintendo Wii U (Win10/11 PGO + Batocera `[AX]`) | [GitHub (`Cemu-Latte-AX`)](https://github.com/galaxywarriordad/Cemu-Latte-AX) | Upstream: [Cemu](https://github.com/cemu-project/Cemu) (`MPL-2.0`) |
| **`FLAC_AX`** | Bare-Metal Linux ESS Sabre Audio & A/V Studio | [GitHub (`FLAC_AX`)](https://github.com/galaxywarriordad/FLAC_AX) | Standalone `libFLAC++` + ALSA (`MIT`) |
| **Amiberry `[AX]`** | Commodore Amiga 500 / 1200 / CD32 Engine | [GitHub (`Amiberry-AX`)](https://github.com/galaxywarriordad/Amiberry-AX) | Upstream: [Amiberry](https://github.com/BlitterStudio/amiberry) (`GPL-3.0`) |
| **Generation-X-AX** | Unified Classic Engines `[AX]` Suite | [GitHub (`Generation-X-AX`)](https://github.com/galaxywarriordad/Generation-X-AX) | `RMG`, `BlastEm`, `Cannonball`, `Flycast`, `Hatari` (`GPL-3.0`) |

---

## 🧩 Core Modules & Aggregated Tooling

* **Unified Settings & Platform Macros (`ax::config`):** Standardized platform/ISA macros (`AX_PLATFORM_WINDOWS`, `AX_PLATFORM_LINUX`, `AX_HAS_AVX2`, `AX_HAS_BMI1`) and zero-dependency `ax_engine.ini` loader (`ax::LoadEngineConfig`) shared across all `[AX]` applications.
* **Lock-Free Atomic Telemetry (`ax::telemetry`):** 64-byte cache-line-isolated atomic counters (`audio_underruns`, `vram_reaps`, `bar1_stalls`, `igpu_offloads`, `qsv_frames_decoded`) exposed via `ax::GetTelemetrySnapshot()` for OSDs and external daemons.
* **Deterministic Topology (`ax::topology`):** Enforces `2:3:1` Non-SMT, dynamic Intel SMT / AMD CCX physical core affinity masks, and `RK3326` `1:2:1` masks alongside real-time thread priorities (`SCHED_FIFO` / MMCSS).
* **Zero-Overhead Memory (`ax::memory`):** `1G`/`2M` HugeTLB and Windows Large-Page fastmem allocators with single-cycle `BMI1` (`_bextr_u64`) page translation.
* **Intel iGPU Offloading (`ax::igpu`):** Offloads hardware video decoding (`QSV` / `VA-API` / `VEBOX`) and compute texture unswizzling onto Intel UHD Graphics.
* **VRAM & BAR1 Safeguards (`ax::vram`):** Proactive `5.00 GiB` async pruning, per-GPU Vulkan VRAM ceilings (`3.50 GiB`–`10.00 GiB` via `config/hardware_profiles.json`), bounded `128 MiB`/`256 MiB` `HOST_VISIBLE` BAR1 staging pools, and runtime `FP16 → FP32` shader promotion.
* **Bit-Perfect Audio Ring (`ax::audio`):** Lock-free, cache-line-aligned PCM ring buffer targeting `2.00 ms` ALSA / `2.67 ms` ASIO exclusive USB DAC playback.
* **Aggregated Host Tooling (`tools/`):** Process-parameterized scripts (`Get-AxProfile.ps1`, `check_hardware.ps1`, `set_irq_affinity.ps1`, `track_telemetry.ps1`, `check_regression.ps1`, `clean_all.ps1`, and Linux `ax_doctor.sh`) for pre-flight audits, IRQ steering, and telemetry collection across `[AX]` targets.

---

## 📂 Repository Structure

```text
ax-sdk/
├── .github/workflows/ # Automated CI validation & multi-platform build checks (ci.yml)
├── ax_toolchain/      # Cross-platform compiler & toolchain presets
├── config/            # Declarative silicon profile matrix (hardware_profiles.json)
├── include/ax/        # Public C++20 headers (config, telemetry, topology, memory, igpu, vram, audio)
├── src/               # Zero-dependency C++20 implementation & OS shims
├── tools/             # Aggregated cross-app host scripts (Get-AxProfile, check_hardware, set_irq_affinity, track_telemetry, check_regression, clean_all, ax_doctor.sh)
├── REPO_TOPOLOGY.md   # 9-repository lineage & Policy-in-SDK boundary contract
├── CI_Maturity.md     # CMM Levels 1–5 quality & Hardware-in-the-Loop roadmap
└── CMakeLists.txt     # CMake target (libax_core / ax::core, v1.36.2)
