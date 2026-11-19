## 🌐 AX Ecosystem: 4-Repository Topology & Lineage

The **AX Project** unifies shared low-level C++20 silicon abstractions (`ax-sdk`) with platform- and emulator-specific forks across Windows 10/11 and bare-metal Linux.

```text
                    +-------------------------------------------------------+

                    |                        ax-sdk                         |
                    |        https://github.com     |
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
| **`ax-sdk`** | **GitHub:** [`galaxywarriordad/ax-sdk`](https://github.com) | Zero-dependency C++20 silicon core (`libax_core`), dynamic SMT/CCX topology resolver, platform shims, and declarative profiles (`config/hardware_profiles.json`). |
| **`Eden-Espresso-Wave-AX`** | **Forgejo:** [`galaxywarriordad/Eden-Espresso-Wave-AX`](https://eden-emu.dev) | **Windows 10/11 Switch Engine Master** — Features LLVM `clang-cl` 23+ 3-stage PGO, FlexASIO/WASAPI Exclusive audio, and Intel QSV offload. |
| **`Piccolo-AX`** | **GitHub:** [`galaxywarriordad/Piccolo-AX`](https://github.com) | **Batocera Linux Branch** — Adapts the `AX` engine for bare-metal Linux (`6.12-rt`) into a `< 300 MB` `toram` EROFS USB appliance via `batocera-ax-external/`. |
| **`Cemu-Latte-AX`** | **GitHub:** [`galaxywarriordad/Cemu-Latte-AX`](https://github.com) | **Nintendo Wii U (Latte GPU) AX Fork** — Integrates `ax::core` for deterministic core isolation and audio/VRAM discipline across Windows and Linux. |

### Cross-Repository Dependency & Merge Flow

* **Shared Silicon Core (`ax-sdk`):** All downstream forks consume `ax-sdk` (`v1.35.0`) via CMake `FetchContent` (`ax::core`), sharing core-pinning, HugeTLB/Large-Page fastmem, and audio ring primitives.
* **Switch Engine Lineage:** Improvements originate in `Eden-Espresso-Wave-AX` (Windows Master) and merge downstream into `Piccolo-AX` (Batocera Linux Branch) via Buildroot overlays.

---

## 🔒 Repository Boundary Contract (Policy-in-SDK / Adapter-in-Consumer)

The ecosystem enforces a strict **Policy-in-SDK / Adapter-in-Consumer** model to eliminate configuration drift:

| Layer | Canonical Repository | Owned Artifacts & Responsibilities |
| :--- | :--- | :--- |
| **Hardware Truth & Guardrails** | **`ax-sdk`** | C++20 runtime primitives (`include/ax/`, `src/`), declarative silicon profile matrix (`config/hardware_profiles.json`, `schema_version: 2.2`), SMT compliance policies, affinity bitmasks, and CI mocks. |
| **Eden Windows Adapter** | **`Eden-Espresso-Wave-AX`** | `clang-cl` PGO orchestration (`PGO_config.bat`), PCIe MSI IRQ steering (`set_irq_affinity.ps1`), log triage, and INI translation via `ax-sdk`. |
| **Cemu Adapter** | **`Cemu-Latte-AX`** | PowerPC JIT + Vulkan 1.4 PGO orchestration and translation mapping `ax-sdk` limits into Cemu `settings.xml`. |
| **Bare-Metal Linux Appliance** | **`Piccolo-AX`** | Buildroot overlay (`batocera-ax-external/`), `6.12-rt` kernel parameters, and isolation/hardware scripts (`S01piccolo-isolate`, `edenGenerator.py`). |

### Enforcement Rules & Maintenance Workflow

1. **No Duplicate Silicon Tables:** Consumer repositories must not maintain local copies of `hardware_profiles.json` or hardcode core/VRAM constants.
2. **Automatic Resolution Chain:** Consumer scripts resolve `ax-sdk/config/hardware_profiles.json` via `$env:AX_SDK_DIR`, sibling checkouts (`../ax-sdk`), or CMake FetchContent caches.
3. **Single-Point Onboarding:** New CPU, GPU, or emulator parameters are introduced once in `ax-sdk/config/hardware_profiles.json` and propagate across all forks.
