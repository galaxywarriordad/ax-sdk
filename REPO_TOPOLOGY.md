# **🌐 The Silicon Preservation Society: 9-Repository Topology & Boundary Contract**

**Programme:** The Silicon Preservation Society (\[AX\])  
**Current SDK Baseline:** ax-sdk v1\_36\_2 (v1.36.2)  
**Last Updated:** 2026-11-24

The **\[AX\] Ecosystem** separates shared low-level C++20 silicon abstractions from platform-, OS-, and emulator-specific repositories so upstream rebases stay clean while hardware isolation logic remains unified across Windows 10/11, bare-metal x86\_64 Linux, and aarch64 handhelds.

# **Paragraph replaced to clear section heading.**

```
                      +-------------------------------------------------------+
                      |                   ax-sdk (v1_36_2)                    |
                      | Shared C++20 Silicon Core, Dynamic SMT/CCX & 1:2:1    |
                      | Topology, Evdev Gamepad Input & SCHED_FIFO Audio      |
                      +-------------------------------------------------------+
                            /             /               \             \
              FetchContent / FetchContent/                 \ FetchContent\ BR2_EXTERNAL
                          v             v                   v             v
+-------------------------+ +-------------------+ +-----------------+ +-------------------+
| Eden-Espresso-Wave-AX   | | Cemu-Latte-AX     | | Amiberry-AX     | | FLAC_AX           |
| (Nintendo Switch [AX])  | | (Wii U / Latte)   | | (Amiga 500/1200)| | S32_LE/DoP & TUI  |
+-------------------------+ +-------------------+ +-----------------+ +-------------------+
                                      |
              +-----------------------+-----------------------+
              |                       |                       |
              v                       v                       v
      +-------------------+   +-------------------+   +-------------------+
      | Generation-X-AX   |   | Piccolo-AX        |   | Piccolo-2go       |
      | (Hatari, BlastEm, |   | (x86_64 6.12-rt   |   | (ARM64 RK3326     |
      |  Flycast, RMG)    |   |  EROFS toram OS)  |   |  1:2:1 Handheld)  |
      +-------------------+   +---------+---------+   +-------------------+
                                        |
                                        v
                              +-------------------+
                              |piccolo-ax-desktop |
                              | (LXQt/X11 Core 5) |
                              +-------------------+
```

# **📦 Repository Breakdown & Lineage (All 9 Repositories)**

| \# | Repository | Host & URL | License | Primary Role & Lineage |
| :---- | :---- | :---- | :---- | :---- |
| **1** | **ax-sdk** | GitHub: `galaxywarriordad/ax-sdk` | `MIT` | Shared C++20 runtime library (`libax_core` / `ax::core`), dynamic Intel SMT / AMD CCX / RK3326 `1:2:1` core topology resolver, `EvdevNavReader` gamepad input, unified settings schema (`ax_engine.ini`), shared-memory atomic telemetry \+ audio jitter profiler (`ax::telemetry`), HugeTLB fastmem, declarative silicon matrix (`config/hardware_profiles.json`, `schema_version: 2.1`), and shared cross-app scripts (`tools/`). |
| **2** | **Piccolo-AX** | GitHub: `galaxywarriordad/Piccolo-AX` | `GPL-3.0` | Standalone `< 512 MBtoram` EROFS Batocera Linux `x86_64` console & A/V appliance (`Linux 6.12-rt`, `PREEMPT_RT`, `NO_HZ_FULL`, `cgroup v2`, `2:3:1` isolation, direct ALSA `SCHED_FIFO` audio, `iHD` VA-API `dma_buf`, `FBNeo` \+ `[AX]` emulator wheel, and `axGenerator.py`). |
| **3** | **Piccolo-2go** | GitHub: `galaxywarriordad/Piccolo-2go` | `GPL-3.0` | Deterministic `aarch64` Buildroot handheld OS (`128 KB`\-block `zstd` SquashFS `< 140 MB`) for **BATLEXP G350** & **R36S** (`RK3326`, `1:2:1` isolation, `1024 MB LZ4` ZRAM, `Panfrost` KMS/DRM, dual SanDisk `SDR50` guard, **HiBy FC3 `1.3 Vrms`** OTG audio). |
| **4** | **piccolo-ax-desktop** | GitHub: `galaxywarriordad/piccolo-ax-desktop` | `GPL-3.0` | On-demand, local-first LXQt/X11 desktop overlay (`< 42 MB` Tier A footprint) for `Piccolo-AX`, isolated on housekeeping Core `5` away from real-time audio/emulation cores. |
| **5** | **FLAC\_AX** | GitHub: `galaxywarriordad/FLAC_AX` | `MIT` | Bit-perfect `S32_LE` / `DoP` audiophile transport (`ES9281PRO` / `ES9281AC Pro`, `hw:CARD=FC3,DEV=0`, `96` frames / `2.00 ms` ALSA `SCHED_FIFO 80`), `libopenmpt` Amiga/Atari ST tracker player, `ax_av_studio`, `ax_video_upscaler`, and `ax_appliance_tui` launcher. |
| **6** | **Amiberry-AX** | GitHub: `galaxywarriordad/Amiberry-AX` | `GPL-3.0` | Deterministic Commodore Amiga 500 / 1200 / CD32 engine linked against `ax-sdk` (`v1_36_2`). |
| **7** | **Generation-X-AX** | GitHub: `galaxywarriordad/Generation-X-AX` | `GPL-3.0` | Unified C++20 `[AX]` adapter suite powering **Hatari** (Atari ST), **BlastEm** (Mega Drive), **Flycast** (Dreamcast/NAOMI), **RMG** (N64), **Cannonball** (OutRun), **EDuke32**, and **Frontier: Elite II**. |
| **8** | **Eden-Espresso-Wave-AX** | Forgejo: `galaxywarriordad/Eden-Espresso-Wave-AX` (GitHub Mirror) | `GPL-3.0` | Deterministic Nintendo Switch `[AX]` engine (`clang-cl` Skylake/BMI2, ThinLTO, 3-stage PGO, Elastic `2:3:1` isolation, FlexASIO/Steinberg ASIO on `Core 0`, `5.00 GiB` async VRAM shield on `Core 1`, ARM64 JIT fibers on `Cores 2–4`, and Intel QSV / ZSTD L18 quarantine on `Core 5`). |
| **9** | **Cemu-Latte-AX** | GitHub: `galaxywarriordad/Cemu-Latte-AX` | `MPL-2.0` | Deterministic Nintendo Wii U (Latte GPU) `[AX]` engine linked against `ax-sdk` (`v1_36_2`) for PowerPC JIT isolation, Intel UHD 630 `VEBOX` video offloading, `5.50 GiB` VRAM protection, and zero-underrun USB DAC audio across Windows PGO and `Piccolo-AX`. |

# **🔒 Repository Boundary Contract (Policy-in-SDK / Adapter-in-Consumer)**

To prevent configuration drift and eliminate duplicated instrumentation across all 9 repositories, the ecosystem enforces a strict **Policy-in-SDK / Adapter-in-Consumer** separation:

| Architectural Layer | Canonical Owner | Owned Artifacts & Responsibilities |
| :---- | :---- | :---- |
| **1\. Silicon Core, Settings & Telemetry** | **ax-sdk** (`include/ax/`, `src/`, `config/`) | C++20 runtime primitives (`topology`, `memory`, `vram`, `igpu`, `audio`, `config`, `telemetry`), declarative silicon matrix (`config/hardware_profiles.json`, `schema_version: 2.1`), unified runtime schema (`ax_engine.ini`), shared-memory telemetry (`Local\AX_Telemetry` / `/dev/shm/ax_telemetry`), and audio jitter histograms. |
| **2\. Cross-App Instrumentation & Host Tooling** | **ax-sdk/tools/** | Shared, process-parameterized host utilities (`Get-AxProfile.ps1`, `check_hardware.ps1`, `set_irq_affinity.ps1`, `track_telemetry.ps1`, `check_regression.ps1`, `clean_all.ps1`, and Linux `ax_doctor.sh`) driven by `hardware_profiles.json` and `ax_engine.ini`. |
| **3\. Eden Switch Adapter** | **Eden-Espresso-Wave-AX** | Switch-specific shader harmonization (`macro_update.ps1`, `shader_validate.ps1`), HLE applet defaults, `Lossless.dll` bridge, and `PGO_config.bat` / `PGO_stage*.bat` orchestration. |
| **4\. Cemu Wii U Adapter** | **Cemu-Latte-AX** | PowerPC JIT \+ Vulkan 1.4 PGO orchestration and translation mapping `ax_engine.ini` / `hardware_profiles.json` into Cemu `settings.xml`. |
| **5\. Standalone Audio & A/V Adapter** | **FLAC\_AX** | `libFLAC++` & `libopenmpt` decoder pipelines, `S32_LE` / `DoP` stereo packing, direct ALSA `hw:CARD=FC3,DEV=0` hardware output loop, `ax_av_studio`, `ax_video_upscaler`, and `ax_appliance_tui`. |
| **6\. Bare-Metal Linux OS & Workbench** | **Piccolo-AX**, **Piccolo-2go** & **piccolo-ax-desktop** | Buildroot `BR2_EXTERNAL` overlays (`batocera-ax-external/`), `6.12-rt` and `RK3326` kernel configs, boot isolation hooks (`S01piccolo-isolate`, `S02piccolo-hw`, `K01piccolo-sync`), shared Qt6 runtime, and unified `axGenerator.py`. |

# **📜 Enforcement & Maintenance Rules**

* **Single Source of Silicon & Settings Truth:** Consumer repositories must never maintain local copies of `hardware_profiles.json` or duplicate core/VRAM/telemetry structs. All `[AX]` binaries load `ax_engine.ini` via `ax::LoadEngineConfig()` and read silicon ceilings from `ax-sdk`.  
* **Unified Shared-Memory Telemetry Contract:** Runtime counters (`audio_underruns`, `vram_reaps`, `bar1_stalls`, `igpu_offloads`, `qsv_frames_decoded`, `audio_jitter`) are published via `ax::InitTelemetryShm()` and read by `ax-sdk/tools/track_telemetry.ps1` across consumers.  
* **Automatic Resolution Chain:** Consumer build scripts resolve `ax-sdk` via `-DFETCHCONTENT_SOURCE_DIR_AX_SDK=<path>`, `$env:AX_SDK_DIR`, sibling checkouts (`../ax-sdk`), or CMake `FetchContent` (`GIT_TAG v1_36_2`, `GIT_SHALLOW TRUE`).  
* **Host-Matched Git Strategy:** Primary forks reside on the same forge as their upstream base (`git.eden-emu.dev` \+ GitHub mirror for Eden; `github.com` for `ax-sdk`, `Piccolo-AX`, `Piccolo-2go`, `piccolo-ax-desktop`, `Cemu-Latte-AX`, `FLAC_AX`, `Amiberry-AX`, and `Generation-X-AX`) using aligned default branches (`master` for Eden; `main` for GitHub repositories).

