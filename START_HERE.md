# START HERE — `ax-sdk` Technical Orientation (`v1_35_1c`)

`ax-sdk` is the shared, zero-dependency C++20 runtime core (`libax_core` / `ax::core`) and declarative silicon policy layer powering the `[AX]` emulator ecosystem (`Eden-Espresso-Wave-AX`, `Piccolo-AX`, and `Cemu-Latte-AX`).

---

## 1. Architectural Boundary Contract

`ax-sdk` enforces a strict **Policy-in-SDK / Adapter-in-Consumer** separation:

* **What belongs in `ax-sdk` (Policy & Primitives):**
  * Zero-dependency C++20 headers and implementations under `include/ax/` and `src/` (`topology`, `memory`, `igpu`, `vram`, `audio`).
  * Declarative hardware profile matrices in `config/hardware_profiles.json` (`schema_version: 2.1`) defining core masks, VRAM ceilings, BAR1 staging limits, and platform constraints.
  * Cross-platform CMake target definitions (`ax::core`) and CI validation (`.github/workflows/ci.yml`).
* **What stays out of `ax-sdk` (Consumer Adapter Responsibilities):**
  * Emulator-specific INI/XML translation, UI/OSD rendering, and game-specific workarounds.
  * Personal or machine-specific hardware overrides (such as local DAC device names or host-specific power/IRQ scripts).
  * Duplicate copies of `hardware_profiles.json` or hardcoded core/VRAM tables inside consumer forks.

---

## 2. Repository Map

```text
ax-sdk/
├── .github/workflows/      # CI validation: JSON check + Linux (Ninja) & Windows (clang-cl) builds
├── ax_toolchain/           # Toolchain definitions and compiler presets
├── config/                 # hardware_profiles.json (schema_version: 2.1) & schema/config assets
├── include/ax/             # Public C++20 headers (topology, memory, igpu, vram, audio)
├── src/                    # C++20 core implementation & OS shims
├── REPO_TOPOLOGY.md        # 4-repository lineage & dependency rules
├── CI_Maturity.md          # Quality & hardware-in-the-loop roadmap
└── CMakeLists.txt          # Builds libax_core and exports the ax::core target
