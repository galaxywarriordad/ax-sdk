# 🌐 AX Ecosystem: 5-Repository Topology & Boundary Contract

**Programme:** The Silicon Preservation Society (`[AX]`)  
**Current SDK Baseline:** `ax-sdk` `v1_36_1` (`v1.36.1`)  
**Last Updated:** `2026-11-20`

The **AX Project** unifies shared low-level C++20 silicon abstractions (`ax-sdk`), cross-application telemetry/auditing tooling (`ax-sdk/tools/`), and declarative runtime settings (`ax_engine.ini` + `hardware_profiles.json`) across Windows 10/11 and bare-metal Linux (`Piccolo-AX`) without duplicating platform code.

```text
                    +-------------------------------------------------------+

                    |                        ax-sdk                         |
                    |     (https://github.com/galaxywarriordad/ax-sdk)      |
                    |  Shared C++20 Silicon Core (libax_core / ax::core),   |
                    |  Unified Settings (ax_engine.ini), SHM Telemetry,     |
                    |  Hardware Profiles (v2.1) & Shared Tools (tools/)     |
                    +-------------------------------------------------------+
                      /                 |                 |                 \
     CMake FetchContent   CMake FetchContent   CMake FetchContent   CMake FetchContent
                    /                   |                 |                   \
                   v                    v                 v                    v
+-------------------------+ +-------------------------+ +-------------------------+ +-------------------------+

| Eden-Espresso-Wave-AX   | |       Piccolo-AX        | |      Cemu-Latte-AX      | |         FLAC_AX         |
| (Forgejo — Switch Fork) | | (GitHub — Batocera OS)  | | (GitHub — Wii U Fork)   | | (GitHub — Audio Player) |
| Windows 10/11 Master,   |>| < 300 MB toram EROFS,   | | Wii U / Latte GPU Fork  | | Bare-Metal Linux ESS    |
| LLVM 23 PGO, ASIO & QSV | | 6.12-rt, ALSA & VA-API  | | Powered by ax::core     | | Sabre ALSA SCHED_FIFO   |
+-------------------------+ +-------------------------+ +-------------------------+ +-------------------------+
```

---

## 📦 Repository Breakdown & Lineage

| Repository | Host & Canonical Repo | License | Primary Role & Lineage |
| :--- | :--- | :--- | :--- |
| **`ax-sdk`** | GitHub: `galaxywarriordad/ax-sdk` | `MIT` | Zero-dependency C++20 silicon core (`libax_core` / `ax::core`), dynamic SMT/CCX topology resolver, unified settings schema (`ax_engine.ini`), shared-memory atomic telemetry + audio jitter profiler (`ax::telemetry`), declarative silicon matrix (`config/hardware_profiles.json`), and shared cross-app scripts (`tools/`). |
| **`Eden-Espresso-Wave-AX`** | Forgejo: `galaxywarriordad/Eden-Espresso-Wave-AX` (GitHub Mirror) | `GPL-3.0` | **Windows 10/11 Switch Engine Master** — Features LLVM `clang-cl` 23+ 3-stage PGO, FlexASIO/Steinberg ASIO (`Core 0`), `5.00 GiB` async VRAM shield (`Core 1`), ARM64 JIT fibers (`Cores 2–4`), and Intel QSV / ZSTD L18 quarantine (`Core 5`). |
| **`Piccolo-AX`** | GitHub: `galaxywarriordad/Piccolo-AX` | `GPL-3.0` | **Batocera Linux Console OS** — Out-of-tree `BR2_EXTERNAL` overlay adapting `[AX]` engines for `Linux 6.12-rt` (`PREEMPT_RT`, `NO_HZ_FULL`, `cgroup v2`) into a `< 300 MB` `toram` EROFS USB 3.0 appliance. |
| **`Cemu-Latte-AX`** | GitHub: `galaxywarriordad/Cemu-Latte-AX` | `MPL-2.0` | **Nintendo Wii U (Latte GPU) Fork** — Integrates `ax::core` for PowerPC JIT isolation, Intel UHD 630 `VEBOX` video offloading, `5.50 GiB` VRAM protection, and zero-underrun USB DAC audio across Windows PGO and `Piccolo-AX`. |
| **`FLAC_AX`** | GitHub: `galaxywarriordad/FLAC_AX` | `MIT` | **Bare-Metal Linux Semi-Pro Audio Appliance** — Deterministic high-resolution FLAC player targeting ESS Sabre ES9281PRO / ES9281AC Pro USB DACs (`hw:CARD=FC3,DEV=0`), pairing `Core 5` non-RT decoding with `Core 0` `SCHED_FIFO 80` ALSA (`96` frames / `2.00 ms`) via `ax::audio::LockFreeStereoRing`. |

---

## 🔒 Repository Boundary Contract (Policy-in-SDK / Adapter-in-Consumer)

To prevent configuration drift and eliminate duplicated instrumentation across 5 repositories, the ecosystem enforces a strict **Policy-in-SDK / Adapter-in-Consumer** separation:

| Architectural Layer | Canonical Owner | Owned Artifacts & Responsibilities |
| :--- | :--- | :--- |
| **1. Silicon Core, Settings & Telemetry** | **`ax-sdk`** (`include/ax/`, `src/`, `config/`) | C++20 runtime primitives (`topology`, `memory`, `vram`, `igpu`, `audio`, `config`, `telemetry`), declarative silicon matrix (`config/hardware_profiles.json`, `schema_version: 2.1`), unified runtime schema (`ax_engine.ini`), shared-memory telemetry (`Local\AX_Telemetry` / `/dev/shm/ax_telemetry`), and audio jitter histograms. |
| **2. Cross-App Instrumentation & Host Tooling** | **`ax-sdk/tools/`** | Shared, process-parameterized host utilities (`Get-AxProfile.ps1`, `check_hardware.ps1`, `set_irq_affinity.ps1`, `track_telemetry.ps1`, `check_regression.ps1`, `clean_all.ps1`, and Linux `ax_doctor.sh`) driven by `hardware_profiles.json` and `ax_engine.ini`. |
| **3. Eden Switch Adapter** | **`Eden-Espresso-Wave-AX`** | Switch-specific shader harmonization (`macro_update.ps1`, `shader_validate.ps1`), HLE applet defaults, `Lossless.dll` bridge, and `PGO_config.bat` / `PGO_stage*.bat` orchestration. |
| **4. Cemu Wii U Adapter** | **`Cemu-Latte-AX`** | PowerPC JIT + Vulkan 1.4 PGO orchestration and translation mapping `ax_engine.ini` / `hardware_profiles.json` into Cemu `settings.xml`. |
| **5. Standalone Audio Adapter** | **`FLAC_AX`** | `libFLAC++` decoder pipeline, `S32_LE` stereo packing, and direct ALSA `hw:CARD=FC3,DEV=0` hardware output loop. |
| **6. Bare-Metal Linux OS** | **`Piccolo-AX`** | Buildroot `BR2_EXTERNAL` overlay (`batocera-ax-external/`), `6.12-rt` kernel config, boot isolation (`S01piccolo-isolate`, `S02piccolo-hw`, `K01piccolo-sync`), and `edenGenerator.py`. |

---

## 📜 Enforcement & Maintenance Rules

1. **Single Source of Silicon & Settings Truth:** Consumer repositories must never maintain local copies of `hardware_profiles.json` or duplicate core/VRAM/telemetry structs. All `[AX]` binaries load `ax_engine.ini` via `ax::LoadEngineConfig()` and read silicon ceilings from `ax-sdk`.
2. **Unified Shared-Memory Telemetry Contract:** Runtime counters (`audio_underruns`, `vram_reaps`, `bar1_stalls`, `igpu_offloads`, `qsv_frames_decoded`, `audio_jitter`) are published via `ax::InitTelemetryShm()` and read by `ax-sdk/tools/track_telemetry.ps1` across `eden`, `Cemu`, and `flac_ax`.
3. **Automatic Resolution Chain:** Consumer build scripts resolve `ax-sdk` via `-DFETCHCONTENT_SOURCE_DIR_AX_SDK=<path>`, `$env:AX_SDK_DIR`, sibling checkouts (`../ax-sdk`), or CMake `FetchContent` (`GIT_TAG v1_36_1`, `GIT_SHALLOW FALSE`).
4. **Host-Matched Git Strategy:** Primary forks reside on the same forge as their upstream base (`git.eden-emu.dev` + GitHub mirror for Eden; `github.com` for `ax-sdk`, `Piccolo-AX`, `Cemu-Latte-AX`, and `FLAC_AX`) using aligned default branches (`master` for Eden; `main` for all GitHub repositories).
