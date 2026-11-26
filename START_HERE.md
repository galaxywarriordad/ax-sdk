# START HERE — `ax-sdk` Technical Orientation (`v1_36_2`)

`ax-sdk` is the shared, zero-dependency C++20 runtime core (`libax_core` / `ax::core`), declarative silicon policy layer, and cross-application host tooling suite powering the **`[AX]`** ecosystem (`Eden-Espresso-Wave-AX`, `Piccolo-AX`, `Piccolo-2go`, `piccolo-ax-desktop`, `Cemu-Latte-AX`, `Amiberry-AX`, `Generation-X-AX`, and `FLAC_AX`).

---

## Deterministic Memory & Diagnostic Principles

* **Explicit Memory Budgets Over AI/ML Guessing:** RAM layout, HugeTLB (`1G` / `2M`) fastmem reserves, LZ4 `zram`, `toram` `/dev/shm` `EROFS` (`< 512 MB`) staging, and `VkVramShield` byte ceilings rely on deterministic rules and fixed budgets rather than dynamic AI/ML guessing or background inference daemons.  
* **Read-Only `ax_doctor` Pre-Flight Role:** `ax_doctor` acts strictly as an on-demand, read-only diagnostic and pre-flight check verifying kernel cmdline isolation flags, `MemAvailable` and HugeTLB/zram/toram headroom, IRQ affinity masks, and DAC/lock readiness without background polling or silent mutation.  
* **Offline Dev/CI Separation:** Any AI or log-parsing tooling remains strictly offline on CPU/RAM outside the target appliance image and outside GPU VRAM.

---

## 1\. Architectural Boundary Contract

`ax-sdk` enforces a strict Policy-in-SDK / Adapter-in-Consumer separation:

* **What belongs in `ax-sdk` (Policy, Primitives & Shared Tooling):**  
  * Zero-dependency C++20 headers and implementations under `include/ax/` and `src/` (`config`, `telemetry`, `topology`, `memory`, `igpu`, `vram`, `audio`).  
  * Cross-process shared-memory telemetry (`Local\AX_Telemetry` / `/dev/shm/ax_telemetry`) and lock-free microsecond audio jitter histograms (`ax::AudioJitterStats`).  
  * Declarative hardware profile matrices in `config/hardware_profiles.json` (`schema_version: 2.1`) defining core masks, VRAM ceilings, BAR1 staging limits, and platform constraints.  
  * Aggregated, process-parameterized host auditing, IRQ steering, workspace purge, telemetry, and read-only pre-flight scripts in `tools/`.  
  * Cross-platform CMake target definitions (`ax::core`) and CI validation (`.github/workflows/ci.yml`).  
* **What stays out of `ax-sdk` (Consumer Adapter Responsibilities):**  
  * Application-specific INI/XML translation (`qt-config.ini`, `settings.xml`), UI/OSD rendering, and game- or codec-specific workarounds.  
  * Personal or machine-specific hardware overrides (such as local DAC device names or local ROM/track paths).  
  * Duplicate copies of `hardware_profiles.json`, shared host scripts, or hardcoded core/VRAM tables inside consumer forks.

---

## 2\. Repository Map

```
ax-sdk/
├── .github/workflows/    # CI validation: JSON check + Linux (Ninja) & Windows (clang-cl) builds
├── ax_toolchain/         # Toolchain definitions and compiler presets
├── config/               # hardware_profiles.json (schema_version: 2.1) & schema/config assets
├── include/ax/           # Public C++20 headers (config, telemetry, topology, memory, igpu, vram, audio)
├── src/                  # C++20 core implementation & OS shims (config.cpp, telemetry.cpp, topology.cpp, memory.cpp)
├── tools/                # Shared cross-app scripts (Get-AxProfile, check_hardware, set_irq_affinity, track_telemetry, check_regression, clean_all, ax_doctor.sh)
├── REPO_TOPOLOGY.md      # 9-repository lineage & dependency rules
├── CI_Maturity.md        # Quality & hardware-in-the-loop roadmap
└── CMakeLists.txt        # Builds libax_core and exports the ax::core target
```

