## 🌐 AX Ecosystem: 4-Repository Topology & Lineage

The **AX Project** separates shared low-level C++20 silicon abstractions (`ax-sdk`) from platform- and emulator-specific forks so upstream rebases stay clean while hardware isolation logic remains unified across Windows 10/11 and bare-metal Linux.

```text
                    +-------------------------------------------------------+
                    |                        ax-sdk                         |
                    |        https://github.com/galaxywarriordad/ax-sdk     |
                    |  Shared C++20 Silicon Core, Dynamic SMT/CCX Topology, |
                    |  Platform Shims, Telemetry Schemas & Master Specs     |
                    +-------------------------------------------------------+
                       /                        |                        \
       CMake FetchContent              CMake FetchContent            CMake FetchContent
                     /                          |                          \
                    v                           v                           v
+----------------------------------+  +----------------------------------+  +----------------------------------+
|     Eden-Espresso-Wave-AX        |  |           Piccolo-AX             |  |          Cemu-Latte-AX           |
|   (Forgejo — Upstream Eden Fork) |  |  (GitHub — Batocera Linux Branch)|  |    (GitHub — Wii U AX Fork)    |
| Windows 10/11 Master Engine,     |->|  Batocera Branch of Espresso Wave|  |  Wii U / Latte GPU Desktop &     |
| LLVM 23 PGO, FlexASIO & QSV      |  |  < 300 MB toram 6.12-rt Appliance|  |  Linux Fork Powered by ax::core  |
+----------------------------------+  +----------------------------------+  +----------------------------------+
```

### Repository Breakdown & Lineage

| Repository | Host & Canonical URL | Primary Role & Lineage |
| :--- | :--- | :--- |
| **`ax-sdk`** | **GitHub:** [`galaxywarriordad/ax-sdk`](https://github.com/galaxywarriordad/ax-sdk) | Shared, zero-dependency C++20 silicon library (`libax_core` / `ax::core`), dynamic Intel SMT / AMD CCX topology resolver, Win32/Linux platform shims, and cross-project master specs. |
| **`Eden-Espresso-Wave-AX`** | **Forgejo:** [`galaxywarriordad/Eden-Espresso-Wave-AX`](https://git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX) | **Windows 10/11 Switch Engine Master** — Direct fork of upstream Eden hosted on Forgejo for frictionless PR cherry-picks; features LLVM `clang-cl` 23+ 3-stage PGO, FlexASIO/WASAPI Exclusive audio, Intel QSV offload, and Maxwell GM200 VRAM shields. |
| **`Piccolo-AX`** | **GitHub:** [`galaxywarriordad/Piccolo-AX`](https://github.com/galaxywarriordad/Piccolo-AX) | **Batocera Linux Branch of `Eden-Espresso-Wave-AX`** — Adapts the Espresso Wave `[AX]` engine for bare-metal Linux (`Linux 6.12-rt`, direct ALSA `SCHED_FIFO`, `iHD` VA-API `dma_buf`) and packages it via `batocera-ax-external/` into a `< 300 MB` `toram` EROFS USB 3.0 console appliance. |
| **`Cemu-Latte-AX`** | **GitHub:** [`galaxywarriordad/Cemu-Latte-AX`](https://github.com/galaxywarriordad/Cemu-Latte-AX) | **Nintendo Wii U (Latte GPU) AX Fork** — Integrates `ax::core` for deterministic physical-core isolation, MMCSS/ALSA audio prioritization, and hardware IRQ/VRAM discipline across Windows 10/11 and `Piccolo-AX`. |

### Cross-Repository Dependency & Merge Flow

* **Shared Silicon Core (`ax-sdk`):** `Eden-Espresso-Wave-AX`, `Piccolo-AX`, and `Cemu-Latte-AX` all consume `ax-sdk` (`v1.35.0`) as a pinned CMake `FetchContent` dependency (`ax::core`), inheriting identical core-pinning, HugeTLB/Large-Page fastmem, and audio ring primitives without duplicating C++20 code.
* **Switch Engine Lineage (`Upstream Eden` → `Eden-Espresso-Wave-AX` → `Piccolo-AX`):** Core Nintendo Switch engine improvements land first in `Eden-Espresso-Wave-AX` (Windows 10/11 Master) and merge downstream into `Piccolo-AX` (Batocera Linux Branch), where the `batocera-ax-external/` Buildroot overlay compiles the Linux target into an immutable `rootfs.erofs` boot image.
