# ⚙️ ax-sdk — Shared C++20 Deterministic Emulation Core

A lightweight, dependency-free C++20 runtime library (`libax_core`) powering **The Silicon Preservation Society** emulators—engineered to keep proven PC hardware out of landfills.

`ax-sdk` provides a unified hardware-abstraction layer so `[AX]` emulator forks can share deterministic CPU core isolation, HugeTLB fastmem, Intel iGPU video/texture offloading, VRAM protection, and bit-perfect low-latency audio without duplicating platform code.

---

## 🔗 The Silicon Preservation Society — Official Repository Network

| Project | Role | Primary Repository | Mirror / Upstream |
| :--- | :--- | :--- | :--- |
| **Piccolo `[AX]`** | `< 300 MB` Bare-Metal Linux Console OS | [GitHub (`Piccolo-AX`)](https://github.com/galaxywarriordad/Piccolo-AX) | Base: [Batocera Linux](https://github.com/batocera-linux/batocera.linux) |
| **`ax-sdk`** | Shared C++20 Runtime Core (`libax_core`) | [GitHub (`ax-sdk`)](https://github.com/galaxywarriordad/ax-sdk) | Dependency-Free Core |
| **Eden Espresso Wave `[AX]`** | Nintendo Switch (Windows 10 Bridge + Linux) | [Eden Forgejo](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX.git) | Mirror: [GitHub](https://github.com/galaxywarriordad/Eden-Espresso-Wave-AX) · Upstream: [Eden](https://git.eden-emu.dev/eden-emu/eden/) |
| **Cemu Latte `[AX]`** | Nintendo Wii U (Win10 PGO + Batocera `[AX]`) | [GitHub (`Cemu-Latte-AX`)](https://github.com/galaxywarriordad/Cemu-Latte-AX) | Upstream: [Cemu](https://github.com/cemu-project/Cemu) |

---

## 🧩 Core Modules

* **Deterministic Topology (`ax::topology`):** Enforces `2:3:1` Non-SMT physical core affinity masks and real-time thread priorities (`SCHED_FIFO` / MMCSS).
* **Zero-Overhead Memory (`ax::memory`):** `1G`/`2M` HugeTLB and Large-Page fastmem allocators with single-cycle `BMI1` (`_bextr_u64`) page translation.
* **Intel iGPU Offloading (`ax::igpu`):** Offloads hardware video decoding (`QSV` / `VA-API` / `VEBOX`) and compute texture unswizzling onto Intel UHD Graphics.
* **VRAM Safeguards (`ax::vram`):** Speculative `5.50 GB` DXGI/Vulkan VRAM shield protecting 6 GB GPUs (such as Maxwell GM200 / Pascal) from allocation spikes.
* **Bit-Perfect Audio Ring (`ax::audio`):** Lock-free, cache-line-aligned PCM ring buffer targeting `2.00 ms` ALSA / ASIO exclusive USB DAC playback.

---

## 📂 Repository Structure

```text
ax-sdk/
├── include/ax/             # Public C++20 headers (topology, memory, igpu, vram, audio)
├── src/                    # Zero-dependency C++20 implementation
├── ax_toolchain/           # Shared Clang 23+ ThinLTO & 3-Phase PGO CMake presets
├── Scripts/                # Cross-platform build & validation helpers
└── CMakeLists.txt          # Static library target (ax::core)
