# ⚙️ ax-sdk — Shared C++20 Deterministic Emulation Core

A lightweight, dependency-free C++20 runtime library (`libax_core` / `ax::core`) powering **The Silicon Preservation Society** emulators—engineered to keep proven PC hardware out of landfills.

`ax-sdk` provides a unified hardware-abstraction layer so `[AX]` emulator forks can share deterministic CPU core isolation, HugeTLB/Large-Page fastmem, Intel iGPU video/texture offloading, VRAM protection, and bit-perfect low-latency audio without duplicating platform code.

---

## 🔗 The Silicon Preservation Society — Official Repository Network

*(See [REPO_TOPOLOGY.md](REPO_TOPOLOGY.md) for the full cross-host dependency diagram.)*

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
* **VRAM Safeguards (`ax::vram`):** Speculative `5.50 GB` (`5.35 GB` under frame generation) DXGI/Vulkan VRAM shield protecting 6 GB GPUs (such as Maxwell GM200 / Pascal) from allocation spikes.
* **Bit-Perfect Audio Ring (`ax::audio`):** Lock-free, cache-line-aligned PCM ring buffer targeting `2.00 ms` ALSA / `2.67 ms` ASIO exclusive USB DAC playback.

---

## 📚 Shared Architecture Specifications (`docs/`)

* [Unified Dual-Target Master Plan (`v1.3`)](docs/UNIFIED_MASTER_PLAN.md)
* [Unified Multi-Architecture Silicon Topology Plan](docs/SILICON_TOPOLOGY_PLAN.md)
* [Unified Subsystem Integration Plan (`v1.1`)](docs/Unified_Subsystem_Integration_1.1.md)
* [Engineering & Industry Benchmark Assessment](docs/Benchmark_Assessment.md)
* [Comprehensive Engineering & Architectural Audit](docs/audits/Comprehensive%20Engineering%20%26%20Architectural%20Audit.md)

---

## 📂 Repository Structure

```text
ax-sdk/
├── include/ax/             # Public C++20 headers (topology, memory, igpu, vram, audio)
├── src/                    # Zero-dependency C++20 implementation
├── config/                 # Unified runtime engine INI schema (eden_engine.ini)
├── scripts/win32/          # Hardware validation, MSI IRQ steering & telemetry scripts
├── docs/                   # Cross-platform master specs, topology plans & audits
└── CMakeLists.txt          # CMake target (libax_core / ax::core)
```

---

## 🤝 Contributing & License

* **Contributing:** See [CONTRIBUTING.md](CONTRIBUTING.md) and [SECURITY.md](SECURITY.md) for compiler hygiene (`clang-cl` 23+ `-Werror`) and hardware-privilege notes.
* **License:** Licensed under the **MIT License** ([LICENSE](LICENSE)) for seamless integration across GPL-3.0, MPL-2.0, and MIT emulator codebases[cite: 30].
