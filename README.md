# ⚙️ ax-sdk — Shared C++20 Deterministic Emulation Core

A lightweight, dependency-free C++20 runtime library (`libax_core` / `ax::core`) powering **The Silicon Preservation Society** emulators—engineered to keep proven PC hardware out of landfills.

`ax-sdk` provides a unified hardware-abstraction layer so `[AX]` emulator forks can share deterministic CPU core isolation, HugeTLB/Large-Page fastmem, Intel iGPU video/texture offloading, VRAM protection, and bit-perfect low-latency audio without duplicating platform code.

---

## 🔗 The Silicon Preservation Society — Official Repository Network

*(See `REPO_TOPOLOGY.md` for the full cross-host dependency diagram and the **Policy-in-SDK / Adapter-in-Consumer** boundary contract.)*

| Project | Role | Canonical Repository | Base / Upstream |
| :--- | :--- | :--- | :--- |
| **`ax-sdk`** | Shared C++20 Runtime Core (`libax_core`) | [GitHub (`ax-sdk`)](https://github.com/galaxywarriordad/ax-sdk) | Dependency-Free C++20 Core |
| **Eden Espresso Wave `[AX]`** | Nintendo Switch (Windows 10/11 PGO + Linux) | [Eden Forgejo (`Eden-Espresso-Wave-AX`)](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX.git) | Upstream: [Eden](https://git.eden-emu.dev/eden-emu/eden/) |
| **Piccolo `[AX]`** | `< 300 MB` Bare-Metal Linux Console OS | [GitHub (`Piccolo-AX`)](https://github.com/galaxywarriordad/Piccolo-AX) | Base: [Batocera Linux](https://github.com/batocera-linux/batocera.linux) |
| **Cemu Latte `[AX]`** | Nintendo Wii U (Win10/11 PGO + Batocera `[AX]`) | [GitHub (`Cemu-Latte-AX`)](https://github.com/galaxywarriordad/Cemu-Latte-AX) | Upstream: [Cemu](https://github.com/cemu-project/Cemu) |

---

## 🧩 Core Modules

* **Deterministic Topology (`ax::topology`):** Enforces `2:3:1` Non-SMT and dynamic Intel SMT / AMD CCX physical core affinity masks alongside real-time thread priorities (`SCHED_FIFO` / MMCSS).
* **Zero-Overhead Memory (`ax::memory`):** `1G`/`2M` HugeTLB and Windows Large-Page fastmem allocators with single-cycle `BMI1` (`_bextr_u64`) page translation.
* **Intel iGPU Offloading (`ax::igpu`):** Offloads hardware video decoding (`QSV` / `VA-API` / `VEBOX`) and compute texture unswizzling onto Intel UHD Graphics.
* **VRAM & BAR1 Safeguards (`ax::vram`):** Per-GPU Vulkan VRAM ceilings and reap triggers (`3.50 GiB`–`10.00 GiB` via `config/hardware_profiles.json`), bounded `128 MiB`/`256 MiB` `HOST_VISIBLE` BAR1 staging pools, and runtime `FP16 → FP32` shader promotion protecting Maxwell (`GM200`/`GM204`), Pascal (`GP102`/`GP104`), and Turing (`TU116`) cards from shared-memory thrashing.
* **Bit-Perfect Audio Ring (`ax::audio`):** Lock-free, cache-line-aligned PCM ring buffer targeting `2.00 ms` ALSA / `2.67 ms` ASIO exclusive USB DAC playback.

---

## 📚 Shared Architecture Specifications

### Core SDK & Ecosystem Contracts (`ax-sdk`)
* [AX Ecosystem Topology & Boundary Contract](REPO_TOPOLOGY.md)
* [CI/CD & Quality Maturity Roadmap](CI_Maturity.md)
* [Declarative Silicon Profile Matrix (`v2.1`)](config/hardware_profiles.json)

### Reference Architecture & Audit Specs (`Eden-Espresso-Wave-AX`)
* [Unified Dual-Target Master Plan (`v1.3`)](https://eden-emu.dev)
* [Unified Multi-Architecture Silicon Topology Plan](https://eden-emu.dev)
* [Unified Subsystem Integration Plan (`v1.1`)](https://eden-emu.dev)
* [Engineering & Industry Benchmark Assessment](https://eden-emu.dev)
* [Comprehensive Engineering & Architectural Audit](https://eden-emu.dev)

---

## 📂 Repository Structure

```text
ax-sdk/
├── .github/workflows/      # Automated CI validation & multi-platform build checks (ci.yml)
├── ax_toolchain/           # Cross-platform compiler & toolchain presets
├── config/                 # Declarative silicon profile matrix (hardware_profiles.json)
├── include/ax/             # Public C++20 headers (topology, memory, igpu, vram, audio)
├── src/                    # Zero-dependency C++20 implementation
├── REPO_TOPOLOGY.md        # 4-repository lineage & Policy-in-SDK boundary contract
├── CI_Maturity.md          # CMM Levels 1–5 quality & Hardware-in-the-Loop roadmap
└── CMakeLists.txt          # CMake target (libax_core / ax::core)
