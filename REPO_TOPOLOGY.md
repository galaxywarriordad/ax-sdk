## 🌐 AX Ecosystem: 4-Repository Topology

The **AX Project** separates shared low-level C++20 silicon abstractions from platform- and emulator-specific forks so upstream rebases stay clean while hardware isolation logic remains unified across Windows 10/11 and bare-metal Linux.

```text
                    +-------------------------------------------------------+
                    |                        ax-sdk                         |
                    |        [https://github.com/galaxywarriordad/ax-sdk](https://github.com/galaxywarriordad/ax-sdk)     |
                    |  Shared C++20 Silicon Core, Dynamic SMT/CCX Topology, |
                    |  Platform Shims, Telemetry Schemas & Master Specs     |
                    +-------------------------------------------------------+
                       /                        |                        \
       CMake FetchContent       Buildroot + CMake FetchContent       CMake FetchContent
                     /                          |                          \
                    v                           v                           v
+----------------------------------+  +----------------------------------+  +----------------------------------+
|     Eden-Espresso-Wave-AX        |  |           Piccolo-AX             |  |          Cemu-Latte-AX           |
|   (Forgejo — Upstream Eden Fork) |  |  (GitHub — Batocera USB 3.0 OS)  |  |    (GitHub — Wii U AX Fork)    |
|  Windows 10/11 Desktop Engine,   |  |  < 300 MB toram EROFS Appliance, |  |  Wii U / Latte GPU Desktop Fork  |
|  LLVM 23 PGO, FlexASIO & QSV     |  |  6.12-rt Kernel, ALSA & VA-API   |  |  Powered by ax::sdk Silicon Core |
+----------------------------------+  +----------------------------------+  +----------------------------------+
```

### Repository Breakdown

| Repository | Host & Canonical URL | Primary Role & Contents |
| :--- | :--- | :--- |
| **`ax-sdk`** | **GitHub:** [`galaxywarriordad/ax-sdk`](https://github.com/galaxywarriordad/ax-sdk) | Shared C++20 silicon library (`ax::sdk`), dynamic Intel SMT / AMD CCX core topology resolver (`topology.cpp`), `#ifdef _WIN32` / `#ifdef __linux__` platform shims, runtime INI schemas, IRQ/telemetry scripts, and cross-project architectural specs. |
| **`Eden-Espresso-Wave-AX`** | **Forgejo:** [`galaxywarriordad/Eden-Espresso-Wave-AX`](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX) | Direct fork of upstream Eden hosted on Forgejo for frictionless upstream PR cherry-picks; Windows 10/11 Nintendo Switch desktop engine featuring LLVM `clang-cl` 23+ 3-stage PGO, FlexASIO/WASAPI Exclusive audio, Intel QSV offload, and Maxwell GM200 VRAM shields. |
| **`Piccolo-AX`** | **GitHub:** [`galaxywarriordad/Piccolo-AX`](https://github.com/galaxywarriordad/Piccolo-AX) | Standalone `< 300 MB` `toram` EROFS Batocera Linux USB 3.0 console appliance; houses the `batocera-ax-external/` Buildroot overlay, `6.12-rt` kernel/GRUB configs, direct ALSA `SCHED_FIFO` audio, and Intel `iHD` VA-API `dma_buf` cross-GPU scripts. |
| **`Cemu-Latte-AX`** | **GitHub:** [`galaxywarriordad/Cemu-Latte-AX`](https://github.com/galaxywarriordad/Cemu-Latte-AX) | Nintendo Wii U (Latte GPU) AX fork integrating `ax::sdk` for deterministic physical-core isolation, MMCSS audio prioritization, and hardware IRQ/VRAM discipline. |

### Cross-Repository Dependency Flow

* **`Eden-Espresso-Wave-AX` & `Cemu-Latte-AX`** consume `ax-sdk` as a pinned CMake `FetchContent` dependency (`ax::sdk`), inheriting identical core-pinning, frame-pacing, and telemetry primitives without duplicating C++20 code.
* **`Piccolo-AX`** pulls immutable, tagged commits of both `Eden-Espresso-Wave-AX` (from Forgejo) and `ax-sdk` (from GitHub) during Buildroot image assembly to produce a deterministic, zero-flash-wear `rootfs.erofs` boot image.