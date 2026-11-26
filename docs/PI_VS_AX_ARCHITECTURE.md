# The Silicon Preservation Society (`[AX]`) vs. Raspberry Pi — Architectural Comparison & Roadmap

*A tale of two philosophies: off-the-shelf single-board convenience versus unapologetic, bare-metal silicon mastery.*

Walk into almost any modern living room or tinkering desk looking for a compact media, retro-gaming, or appliance box and the **Raspberry Pi** is the default answer. It is cheap, cheerful, supported by an army of tutorials, and happy to juggle a desktop environment, background services, and a media player all at once.

**The Silicon Preservation Society (`[AX]`)** plays a very different game. Rather than buying new single-board computers or conceding capable mid-tier x86\_64 PCs and budget RK3326 handhelds to e-waste, `[AX]` treats existing hardware as precision instruments—stripping away OS jitter, locking audio and frame pacing to dedicated CPU cores, and feeding external USB DACs bit-perfect streams without software resampling.

Where a general-purpose Raspberry Pi setup behaves like a Swiss Army knife running in the background, the `[AX]` stack behaves like a dedicated hi-fi transport and console appliance: **Core 0** is guarded for `SCHED_FIFO` audio and low-latency IRQs, **Core 1** handles display scanout, middle cores shoulder emulation or DSP synthesis, and **Core 5** (or **Core 3** on `RK3326`) quietly handles I/O, preloading, and encoding.

---

## Deterministic Memory Governance & Read-Only `ax_doctor` Role

* **Explicit Memory Budgets Over AI/ML Guessing:** RAM layout, `1G`/`2M` HugeTLB reserves, LZ4 `zram`, `toram` `EROFS`/`SquashFS` budgets (`< 512 MB` on `Piccolo-AX`, `< 140 MB` `zstd` SquashFS on `Piccolo-2go`), and `VkVramShield` ceilings follow fixed, deterministic budgets rather than dynamic AI/ML guessing or background inference daemons that would consume RAM/VRAM or introduce jitter on isolated cores.  
* **Read-Only `ax_doctor` Pre-Flight Role:** `ax_doctor` acts strictly as an on-demand, read-only diagnostic and pre-flight check verifying kernel isolation cmdline flags, `MemAvailable` and HugeTLB/zram/toram headroom, IRQ affinity masks, and DAC / lock readiness without background polling or silent mutation.  
* **Offline Dev/CI Separation:** Any AI or log-parsing tooling stays strictly offline on CPU/RAM outside the live appliance image and outside GPU VRAM.

---

## Performance Differences: `[AX]` Ecosystem vs. Raspberry Pi

| Dimension | Silicon Preservation Society (`[AX]`) | Raspberry Pi (Typical General-Purpose Setup) |
| :---- | :---- | :---- |
| **Audio Timing & Bit-Perfection** | **Class-leading determinism:** `SCHED_FIFO` (priority 80\) on dedicated Core 0, `S32_LE` / `DoP` (`DSD64` / `DSD128`) bypasses desktop mixers/resampling, paired with `mlockall` RAM locking and external USB-C DACs (such as the HiBy FC3). | **Good for everyday listening, prone to contention:** Standard Linux audio stacks and shared CPU cores can experience occasional scheduling jitter under heavy I/O or background tasks unless carefully tuned. |
| **Frame Pacing & Emulation Headroom** | **Purpose-built core partitioning:** `2:3:1` isolation on 6-core `x86_64` (`i5-9600K` / `i5-8600K`) with `HugeTLB` fastmem, VRAM guardrails (`5.50 GiB` ceiling), and `1:2:1` isolation on `RK3326` handhelds (`< 140 MB` `zstd` SquashFS). | **Broad compatibility, shared resources:** Great for classic 8-bit/16-bit/arcade and light 3D workloads, but threads compete across cores without strict appliance-level isolation by default. |
| **System Footprint & Boot Behavior** | **Lean, RAM-resident appliances:** `< 512 MB` `toram` EROFS (`Piccolo-AX` on `Linux 6.12-rt`) and `< 42 MB` quarantined X11 desktop (`piccolo-ax-desktop`) minimize disk thrash and background noise. | **Full-featured general OS:** Easy to expand with packages and desktop apps, at the cost of more background daemons and higher memory/storage overhead. |
| **Hardware Longevity & Value** | **Anti-obsolescence by design:** Extends the life of existing PCs, GPUs, and handhelds rather than requiring new SBC purchases. | **Purpose-built new hardware:** Low entry cost for new projects, though still subject to board-level I/O and GPU bottlenecks on heavier tasks. |

**The Verdict:** If you want plug-and-play simplicity and broad general-purpose tinkering out of the box, the Raspberry Pi remains an easy recommendation. If your priority is uncompromising clock stability, zero-jitter audio, deterministic frame pacing, and keeping proven silicon out of landfills, The Silicon Preservation Society is in a league of its own.

---

# The Silicon Preservation Society (`[AX]`): Inside a Bare-Metal Architecture Engineered to Rescue Proven Silicon from Landfills

**A comprehensive technical monograph examining every repository, binary, bootloader sequence, memory map, desktop workbench, and real-time audio/video/emulation pipeline across the nine-repository `[AX]` ecosystem.**

# Executive Overview: Anti-Obsolescence Through Silicon Determinism

Every year, millions of functional desktop processors, discrete graphics cards, and compact ARM systems-on-chip are discarded—not because the silicon has degraded, but because modern general-purpose operating systems bury hardware beneath layers of background telemetry, dynamic frequency jitter, software audio resampling, and uncoordinated thread migration. When an operating system allows browser notifications, indexers, desktop compositors, and storage interrupts to wander freely across every CPU core, a six-core desktop or quad-core handheld behaves sluggishly even when its raw arithmetic and graphics throughput remain entirely capable.

The Silicon Preservation Society (`[AX]`)—architected and maintained by galaxywarriordad across GitHub and Forgejo—approaches this problem from first principles. Rather than treating software optimization as an afterthought or buying new single-board computers to replace older desktops, the `[AX]` ecosystem treats proven legacy hardware as deterministic embedded appliances. Its target reference hardware spans two distinct instruction set architectures:

1. **Stream 1 Desktop & Console Reference (`x86_64`):** Intel Core `i5-9600K` (6 physical cores / 6 threads, Hyper-Threading disabled, locked at `4.8 GHz` with 2 MB L3 cache per core) and Intel Core `i5-8600K`, paired with `32 GB DDR4-3600`, an `NVIDIA GTX 980 Ti` (`6 GB` GM200 Maxwell), integrated `Intel UHD Graphics 630` (`Quick Sync Video` / `iHD` VA-API), and external USB Audio Class 2.0 DACs led by the `HiBy FC3` (`ESS Sabre ES9281AC Pro` / `ES9281PRO`).  
2. **Stream 2 Handheld Reference (`aarch64`):** `BATLEXP G350` and `R36S` retro handheld consoles powered by the `Rockchip RK3326` (`4x ARM Cortex-A35`, `ARM Mali-G31 MP2`, `RGA2-Lite`, `1 GB LPDDR3`, and `RK817` PMIC/codec), running dual high-endurance SanDisk Extreme PRO microSD storage (`TF1` 64 GB OS \+ `TF2` 128 GB media library) and `HiBy FC3` (`1.3 Vrms` early revision) USB-C OTG audio.

Across nine interconnected repositories, the ecosystem enforces a strict architectural contract: **Policy-in-SDK / Adapter-in-Consumer**. Hardware detection, physical core partitioning, huge-page fastmem allocation, GPU VRAM ceilings, and lock-free telemetry reside in a single zero-dependency C++20 core (`ax-sdk`), while bare-metal OS images (`Piccolo-AX`, `Piccolo-2go`), an isolated X11 workbench (`piccolo-ax-desktop`), and domain-specific media and emulation engines (`FLAC_AX`, `Eden-Espresso-Wave-AX`, `Cemu-Latte-AX`, `Amiberry-AX`, `Generation-X-AX`) consume those policies without duplicating low-level platform code.

# Chapter 1: Bootloader Mechanics, Kernel Isolation, and RAM-Resident Execution Speed

One of the defining characteristics of the `[AX]` ecosystem is how quickly systems transition from cold power-on to deterministic execution—and how completely they eliminate storage and scheduler interference once running.

## 1.1 The `x86_64` Boot Sequence (`Piccolo-AX` via Limine & GRUB)

On `x86_64` hardware, `Piccolo-AX` boots through Limine (`limine.conf`) or GRUB (`grub.cfg`), presenting a multi-mode appliance menu capable of launching straight into isolated single-workload console/studio environments or the opt-in X11 desktop workbench. Rather than mounting a writable root filesystem full of desktop daemons, the bootloader loads a pruned, compressed `< 512 MB` EROFS (`lz4hc,9`) image directly into physical RAM via `toram` alongside a **`Linux 6.12-rt` (`PREEMPT_RT`)** real-time kernel.

At the kernel command line and early userspace initialization (`S01piccolo-isolate` and `S02piccolo-hw`), the boot sequence locks down the silicon before any emulator or audio engine initializes:

* **Tickless Core Partitioning (`2:3:1` Topology):** Kernel flags (`isolcpus`, `nohz_full`, `rcu_nocbs`, and `irqaffinity`) carve the six physical cores into four non-overlapping execution domains:  
  * **Core 0 (`0x01`):** Guarded exclusively for `SCHED_FIFO` (Priority `80`) real-time audio clock submission and xHCI USB DAC hardware interrupts.  
  * **Core 1 (`0x02`):** Isolated for Vulkan, OpenGL, and DRM/KMS scanout and VBlank-locked frame presentation.  
  * **Cores 2–4 (`0x1C`):** Dedicated exclusively to guest CPU/JIT execution (Switch, Wii U, Amiga, N64, Dreamcast, Mega Drive, Atari ST, Arcade) or multi-voice AVX2/FMA DSP synthesis.  
  * **Core 5 (`0x20`):** Designated as the housekeeping, storage I/O, RAM preloading, MIDI polling, TUI launcher, batch video encoding, and opt-in X11 desktop plane.  
* **cgroup v2 & Memory Protection:** `S01piccolo-isolate` and `S02piccolo-hw` establish `cpuset` hierarchies, configure `LZ4` `ZRAM` swap, enable `HugeTLB` pages, and enforce a strict **`5.50 GiB` VRAM ceiling** on the 6 GB `GTX 980 Ti` so driver-level memory overcommit never triggers PCIe BAR1 thrashing.  
* **Volatile RAM Staging with Atomic Persistence:** Because the root filesystem runs `toram` in read-only `EROFS`, runtime shader caches, PGO profiles, and NVIDIA shader caches (`emu_cache`, `pgo`, `nv_cache`) operate at DDR4-3600 memory speeds inside `/dev/shm`, while `K01piccolo-sync` atomically archives them to `.tar.zst` on persistent `SHARE` storage (`/userdata`) during clean shutdown.

## 1.2 The `aarch64` Handheld Boot Sequence (`Piccolo-2go` via U-Boot & ATF BL31)

On the `BATLEXP G350` and `R36S`, `Piccolo-2go` replaces general-purpose handheld distributions with a deterministic embedded boot chain engineered around the **Rockchip RK3326** (`28nm HKMG`) and **RK817** PMIC:

1. **SPL / TPL (`idbloader.img` at raw 32 KB offset) & `u-boot.itb` (`ATF BL31` at raw 8 MB offset):** Initializes the `32-bit` `1 GB LPDDR3` memory controller at `720 MHz` (`1440 MT/s`, `5.76 GB/s` peak bandwidth) and loads the custom `640x480` espresso-and-copper splash screen (`board/piccolo-2go/logo.bmp`).  
2. **Profile-Driven Device Tree & Voltage Rail Staging (`CMakeLists.txt`):** `Piccolo-2go` stages console-specific `.dtb` files and `rk3326-oc-voltage.dtbo` overlays aligned to `12.5 mV` (`12500 uV`) `RK817` `VDD_LOGIC` steps across three explicit operating tiers:  
   * **`safe` Profile:** `1296 MHz` CPU / `520 MHz` GPU / `666 MHz` DDR @ `1.1000 V` (`1100000 uV`).  
   * **`default` Profile:** `1344 MHz` CPU / `600 MHz` GPU / `720 MHz` DDR @ `1.1375 V` (`1137500 uV`).  
   * **`oc_boost` Profile:** `1392 MHz` CPU (`1344 MHz` boot) / `620 MHz` GPU / `786 MHz` DDR @ `1.1625 V` (`1162500 uV`).  
3. **Immutable `zstd` SquashFS & `1:2:1` IRQ Pinning (`S01piccolo-2go-isolate`):** The kernel mounts a `< 140 MB` `128 KB`\-block `zstd` SquashFS root filesystem (`TF1` partition 2\) alongside `1024 MB` of `LZ4` `ZRAM` (`vm.swappiness=120`, `vm.page-cluster=0`). Boot flags (`isolcpus=0,1,2 nohz_full=0,1,2 rcu_nocbs=0,1,2 threadirqs`) and `S01piccolo-2go-isolate` pin hardware IRQs and threaded `irq/<N>-*` handlers across the four Cortex-A35 cores:  
   * **Core 0 (`0x01`):** `SCHED_FIFO 80` ALSA `S32_LE` (`192`\-frame / `4.00 ms` period), `DWC2` USB-C OTG (`HiBy FC3`, `bInterval=4` microframe coalescing), `I2S0` (`RK817`), and `GPIO`/`SARADC` control IRQs.  
   * **Core 1 (`0x02`):** `Mali-G31 MP2` (`Panfrost` GLES 3.0), fixed-function `RGA2-Lite` `2x` integer scaler, and atomic `KMS/DRM` page flips (`PAN_MESA_DEBUG=noafbc`) to the `640x480 @ 60.00 Hz` OCA-laminated IPS panel.  
   * **Core 2 (`0x04`):** Deterministic `FBNeo` / DSP worker with hot `32 KB` L1 caches, `mlock()` audio rings, and `LD_PRELOAD` `2 MB` HugePage backing (`ax_hugepage_shim.so`).  
   * **Core 3 (`0x08`):** `dwmmc` (`TF1` 64 GB \+ `TF2` 128 GB `SDR50`) DMA IRQs, `1024 MB LZ4` ZRAM, `64 MiB` `FLAC_AX` preloader, and `ax-powerd` thermal/battery guard.

## 1.3 Why Everything Loads and Runs So Fast

Across both platforms, load times and frame-to-frame execution speeds are governed by three compounding mechanisms:

1. **Zero Storage Bus Contention During Playback and Gameplay:** On `Piccolo-AX`, the entire OS resides in RAM (`toram`), and `FLAC_AX` preloads entire lossless albums (up to an `8 GiB` `WholeAlbum` ceiling on `32 GB` systems) into `mlockall`\-pinned physical memory before streaming begins. Once playback or emulation starts, NVMe and SD controllers go virtually idle.  
2. **Shared Dynamic Libraries & Stripped Binaries:** In `Piccolo-AX`, Qt6 is built dynamically (`BR2_SHARED_LIBS=y`) with full symbol stripping (`BR2_STRIP_strip=y`), shrinking Qt6 binaries by over `60%` and allowing `Eden-Espresso-Wave-AX`, `Cemu-Latte-AX`, and `RMG` to share a single copy of `libQt6Core.so`, `libQt6Gui.so`, and `libQt6Widgets.so` in RAM.  
3. **Zero-Allocation Hot Paths & Compiler PGO:** Critical loops in `FLAC_AX` and `ax-sdk` use preallocated `std::array` structures, lock-free SPSC ring buffers, and cache-line-aligned atomics (`alignas(64)`), while Windows/Linux emulator builds leverage LLVM `clang-cl` / Clang `-march=skylake`, ThinLTO, and 3-stage Profile-Guided Optimization (PGO).

# Chapter 2: How `ax-sdk` and `FLAC_AX` Power the Entire Ecosystem

## 2.1 `ax-sdk` (`v1_36_2`): The Shared C++20 Silicon Control Plane

`ax-sdk` (`libax_core` / `ax::core`, MIT-licensed) is the architectural bedrock of The Silicon Preservation Society. Designed with zero external library dependencies—linking only against `Threads`, `dl`, and `rt` on Linux, or `winmm`, `avrt`, and `psapi` on Windows—it ensures that all downstream repositories share identical hardware policies without copy-pasting platform code:

* **Topology Discovery & Thread Affinity (`include/ax/topology.hpp`, `src/topology.cpp`):** Exposes `ax::topology::Domain` (`Audio`, `Vulkan`, `Jit`, `Housekeeping`), `SiliconProfile`, `ProbeHostSiliconOnce()`, `DetectHostSilicon()`, `PinCurrentThread()`, `pin_thread_to_core()`, and `pin_to_housekeeping_core5()`. Whether running on a 6-core `i5-9600K` (`2:3:1`), an SMT/CCX desktop CPU, or a quad-core `RK3326` (`1:2:1`), consumers query a single canonical API.  
* **Unified Configuration & Silicon Profiles (`include/ax/config.hpp`, `config/hardware_profiles.json`):** Standardizes platform/ISA detection macros (`AX_PLATFORM_WINDOWS`, `AX_PLATFORM_LINUX`, `AX_HAS_AVX2`, `AX_HAS_BMI1`), parses `ax_engine.ini` via `ax::LoadEngineConfig`, and resolves GPU/CPU ceilings from `hardware_profiles.json` (`schema_version: 2.1`).  
* **Zero-Overhead Memory & VRAM Guardrails (`include/ax/memory.hpp`, `include/ax/vram.hpp`, `include/ax/igpu.hpp`):** Provides `1G` / `2M` `HugeTLB` and Windows Large-Page fastmem allocators with single-cycle `BMI1` (`_bextr_u64`) page translation, proactive `5.00–5.50 GiB` Vulkan VRAM pruning, bounded `128–256 MiB` `HOST_VISIBLE` BAR1 staging pools, `GM200` `FP16 -> FP32` shader promotion, and Intel `QSV` / `VA-API` / `VEBOX` iGPU offloading.  
* **Lock-Free Audio Ring & Shared-Memory Telemetry (`include/ax/audio.hpp`, `include/ax/telemetry.hpp`):** Supplies cache-line-aligned PCM ring buffers and atomic telemetry counters (`audio_underruns`, `vram_reaps`, `bar1_stalls`, `igpu_offloads`, `qsv_frames_decoded`) mapped to `Local\AX_Telemetry` (Windows) or `/dev/shm/ax_telemetry` (Linux).  
* **Cross-App Host Tooling (`ax-sdk/tools/`):** Includes `Get-AxProfile.ps1`, `check_hardware.ps1`, `set_irq_affinity.ps1`, `track_telemetry.ps1`, `check_regression.ps1`, `clean_all.ps1`, and read-only pre-flight `ax_doctor.sh` so pre-flight hardware checks and IRQ steering work uniformly across `eden`, `Cemu`, `Piccolo-AX`, and `FLAC_AX`.

## 2.2 `FLAC_AX` (`v1.36.2`): The Audio Clock Master and Cross-Appliance Orchestrator

While `ax-sdk` provides the C++20 silicon primitives, **`FLAC_AX`** serves as the ecosystem's reference real-time audio/AV engine and appliance control plane:

* **Hardware DAC Clock Dominance:** `FLAC_AX` treats the external UAC 2.0 DAC (`HiBy FC3`, `TRN Black Pearl`, `FiiO KA11`, or `Moondrop Dawn Pro 2`) as the system's master clock. By disabling ALSA rate resampling (`snd_pcm_hw_params_set_rate_resample(0)`), bypassing `PipeWire` and `PulseAudio`, and locking buffers in RAM via `mlockall`, it eliminates software clock drift and sample-rate conversion artifacts.  
* **Exclusive DAC Arbitration (`/run/lock/ax-dac.lock`):** Because `FLAC_AX` and `[AX]` emulators both demand direct hardware access to `hw:CARD=FC3,DEV=0`, `FLAC_AX` coordinates an advisory `flock(LOCK_EX | LOCK_NB)` lock file at `/run/lock/ax-dac.lock` and cleanly surrenders the DAC on `SIGUSR1`, `SIGTERM`, or `SIGINT` whenever an emulator launches.  
* **Cross-Device Media Pipeline (`ax_encode_2go`):** `FLAC_AX` and `Piccolo-AX` bridge the desktop workstation and the `RK3326` handheld via `ax_encode_2go`—a Core 5 (`pin_to_housekeeping_core5()`) batch transcoding pipeline that outputs pixel-exact `640x480` Main-profile H.264 streams (zero B-frames) with `48000 Hz` stereo audio so `Piccolo-2go` plays video with zero on-device scaling or audio resampling.

# Chapter 3: Repository-by-Repository Deep Dive (All 9 Repositories)

## 1\. `ax-sdk` (`github.com/galaxywarriordad/ax-sdk`)

`ax-sdk` is the dependency-free C++20 shared runtime core (`libax_core` / `ax::core`, pinned at `v1_36_2`) that binds the entire Silicon Preservation Society ecosystem together. Stored across `include/ax/`, `src/`, `config/hardware_profiles.json` (`schema_version: 2.1`), `ax_toolchain/`, `docs/`, and `tools/`, it implements the Policy-in-SDK / Adapter-in-Consumer boundary so every downstream emulator and audio tool inherits identical 2:3:1 and `1:2:1` core affinity masks, `HugeTLB` fastmem allocators, Maxwell `5.50 GiB` VRAM guardrails, and lock-free shared-memory telemetry without duplicating platform code.

## 2\. `Piccolo-AX` (`github.com/galaxywarriordad/Piccolo-AX`)

`Piccolo-AX` is a bare-metal `x86_64` Buildroot/Batocera Linux (`Linux 6.12-rt` `PREEMPT_RT`) console and audio/visual appliance engineered to boot a pruned `< 512 MB` `EROFS` (`lz4hc,9`) image straight into RAM (`toram`). Through its `batocera-ax-external/` `BR2_EXTERNAL` tree, boot-time isolation scripts (`S01piccolo-isolate`, `S02piccolo-hw`, `K01piccolo-sync`), bootloader mode selector (`limine.conf` / `ax-mode-cli`), and shared dynamic Qt6 library packaging (`BR2_SHARED_LIBS=y`), it turns preserved Coffee Lake and Maxwell PCs into zero-flash-wear, instant-response appliances packaging `Eden`, `Cemu`, `Amiberry`, `Generation-X-AX`, and `FLAC_AX`.

## 3\. `Piccolo-2go` (`github.com/galaxywarriordad/Piccolo-2go`)

`Piccolo-2go` is a deterministic `aarch64` Buildroot operating system (`128 KB`\-block `zstd` `SquashFS` `< 140 MB`) tailored specifically for the `BATLEXP G350` and `R36S` retro handhelds on `Rockchip RK3326` silicon (`4x Cortex-A35`, `Mali-G31 MP2`, `1 GB LPDDR3`, `RK817` PMIC). By enforcing `1:2:1` core and IRQ isolation (`S01piccolo-2go-isolate`), `12.5 mV`\-stepped safe / default / `oc_boost` voltage-frequency profiles (`CMakeLists.txt`), `RGA2-Lite` `2x` hardware integer scaling, `1024 MB LZ4` ZRAM, `ax-powerd` battery/thermal guards, and `HiBy FC3` (`1.3 Vrms`) USB-C OTG audio, it extracts rock-solid `60.00 Hz` arcade frame pacing (`FBNeo`), `640x480` `mpv` playback, and bit-perfect `FLAC_AX` transport from a 1 GB handheld.

## 4\. `piccolo-ax-desktop` (`github.com/galaxywarriordad/piccolo-ax-desktop`)

`piccolo-ax-desktop` is an opt-in, ultra-lean (`< 42 MB` footprint, `< 35 MB` stripped) native C/C++ X11 desktop workbench staged as a pluggable `BR2_EXTERNAL` overlay for `Piccolo-AX`. Structured into a core `< 42 MB` profile (`profiles/desktop-core.mk`) and an optional heavy creative/security tier (`profiles/desktop-heavy.mk`), it quarantines general-purpose desktop computing onto housekeeping **Core 5** with persistent `SHARE` storage so workstation utilities never contaminate real-time audio or emulation cores.

## 5\. `FLAC_AX` (`github.com/galaxywarriordad/FLAC_AX`)

`FLAC_AX` is a zero-resampling, RAM-resident C++20 audio and audio-visual runtime built around the `2:3:1` topology (`-march=skylake`, LTO, `ax::core` linkage at `v1_36_2`) that houses four specialized binaries in one repository (`flac_ax`, `ax_av_studio`, `ax_video_upscaler`, and `ax_appliance_tui`, plus `ax_encode_2go`). Spanning bit-perfect `S32_LE` and `DoP` (`DSD64`/`DSD128`) album playback, `libopenmpt` Amiga/Atari ST tracker playback, a 2.00 ms 16-voice polyphonic synthesizer with Akai MPK Mini MK3 RawMIDI/OLED integration, a V4L2/DVD OpenGL 3.3 `Y`/`UV` video upscaler, and a Core 5 Espresso TUI launcher, it anchors both hi-fi listening and appliance orchestration.

## 6\. `Eden-Espresso-Wave-AX` (`git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX`)

`Eden-Espresso-Wave-AX` is the flagship Nintendo Switch emulation fork—maintained locally on Windows 10/11 and hosted on Forgejo (`git.eden-emu.dev`) with a GitHub mirror—engineered to push preserved `i5-9600K` \+ `GTX 980 Ti` hardware to modern console performance. Compiled with LLVM `clang-cl` (`Skylake`/`BMI2`), `ThinLTO`, and 3-stage Profile-Guided Optimization (`Scripts/PGO_config.bat`, `PGO_stage0I.bat`, `PGO_stage1.bat`, `PGO_stage2.bat`), it pairs Elastic `2:3:1` core isolation and `FlexASIO`/exclusive audio with Intel `QSV` video offloading and a `5.00–5.50 GiB` Maxwell VRAM ceiling, while also compiling into `Piccolo-AX` (`eden-ax`) with `ax-sdk` at `v1_36_2` for bare-metal Linux execution.

## 7\. `Cemu-Latte-AX` (`github.com/galaxywarriordad/Cemu-Latte-AX`)

`Cemu-Latte-AX` is a deterministic Nintendo Wii U (`Latte` GPU) fork (`MPL-2.0`) integrating `ax::core` (`v1_36_2`) across both Windows PGO builds and `Piccolo-AX` (`cemu-latte-ax`). By isolating PowerPC JIT execution across **Cores 2–4**, pinning Vulkan/OpenGL submission to **Core 1** and exclusive USB DAC audio to Core 0, offloading video work to Intel UHD 630 VEBOX, and enforcing the 5.50 GiB VRAM ceiling, it eliminates shader/memory stutters and audio underruns on mid-tier legacy desktops.

## 8\. `Amiberry-AX` (`github.com/galaxywarriordad/Amiberry-AX`)

`Amiberry-AX` is a deterministic Commodore Amiga (500, 1200, CD32, OCS/ECS/AGA) fork linked against `ax-sdk` (`v1_36_2`) and packaged into `Piccolo-AX` (`amiberry-ax`). Applying 2:3:1 thread affinity and `HugeTLB` fastmem to cycle-sensitive `50 Hz` PAL Amiga workloads prevents host scheduler jitter from disturbing copper effects, smooth scrolling, and Paula audio timing.

## 9\. `Generation-X-AX` (`github.com/galaxywarriordad/Generation-X-AX`)

`Generation-X-AX` is a unified C++20 `[AX]` adapter suite (`ax_retro_bridge` / `ax::core`, `v1_36_2`) consolidating seven classic 1985–1998 computer, console, and arcade engines—`RMG` (Nintendo 64), `BlastEm` (Sega Mega Drive), `Cannonball` (Sega OutRun), `EDuke32`, `Frontier: Elite II` (`glfrontier`), `Flycast` (Sega Dreamcast/NAOMI/Atomiswave), and `Hatari` (Atari ST/STE/TT/Falcon)—under a single silicon-isolation and memory contract. Rather than maintaining seven divergent platform forks, `Generation-X-AX` gives every classic engine identical `2:3:1` / `1:2:1` affinity, low-latency DAC routing, and clean upstream rebase boundaries.

# Chapter 4: Batocera `[AX]` Desktop (`piccolo-ax-desktop`) vs. Raspberry Pi OS

When users need a compact desktop environment alongside a console or media box, **Raspberry Pi OS** (running LXDE/Pixel or Wayfire on Debian) is the conventional benchmark. `piccolo-ax-desktop` takes a radically different systems-engineering approach: instead of running a desktop continuously on top of real-time workloads, it builds an immutable, two-tier C/C++ X11 workbench overlay (`BR2_EXTERNAL_PICCOLO_AX_DESKTOP_PATH`) that boots on demand via `ax-mode-cli` / Limine and stays strictly quarantined on housekeeping **Core 5** (`0x20`).

## 4.1 Two-Tier Package Architecture

1. **Core Desktop (`profiles/desktop-core.mk` — `< 42 MB` Total Budget):**  
   * **Session & File Management:** Lightweight `LXQt` / `XFCE` session, `PCManFM-Qt` (`AX_PKG_PCMANFM_QT`), `Worker` (`AX_PKG_WORKER`), and `LXQt Archiver` (`AX_PKG_LXQT_ARCHIVER`).  
   * **Developer & Shell Basics:** `Git`, CLI `diff`/`patch` tools, `ripgrep`, and archive utilities including `lhasa` (`AX_PKG_LHASA`) for native Commodore Amiga `.lha`/`.lzh` archives.  
   * **Browser:** `Falkon` (Qt-native C++ browser with built-in ad-blocking, avoiding Chromium/Electron memory bloat).  
   * **Media & YouTube:** `mpv` \+ `yt-dlp` with hardware video decoding and safe software fallback via `scripts/launch-yt.sh`.  
   * **Office & Reading:** `AbiWord`, `Gnumeric`, `Foliate`, and lightweight PDF viewers (`qpdfview`, `MuPDF`, `Atril`).  
   * **2D & Pixel Graphics Suite (`< 42 MB` total):** `mtPaint` (`AX_PKG_MTPAINT`), `GrafX2` (`AX_PKG_GRAFX2`), `AzPainter` (`AX_PKG_AZPAINTER`), and `LibreSprite` (`AX_PKG_LIBRESPRITE`).  
   * **Classic Audio & Trackers:** `MilkyTracker` (`AX_PKG_MILKYTRACKER`), `mhWaveEdit` (`AX_PKG_MHWAVEEDIT`), and `QASMixer` (`AX_PKG_QASMIXER`).  
   * **Diagnostics & Inspection:** `GHex` (`AX_PKG_GHEX`), `feh`, `scrot`, `Qalculate! Qt` (`AX_PKG_QALCULATE_QT`), `CPU-X` (`AX_PKG_CPUX`), `nvtop` (`AX_PKG_NVTOP`), and `htop` (`AX_PKG_HTOP`).  
2. **Optional Heavy Tier (`profiles/desktop-heavy.mk`):**  
   * **Graphics & Video:** `GIMP`, `Krita`, `Shotcut`, `Kdenlive`, and `HandBrake`.  
   * **Security:** `KeePassXC` stored locally on persistent `SHARE` storage (`/userdata`).

## 4.2 Side-by-Side Comparison: `piccolo-ax-desktop` vs. Raspberry Pi OS

| Capability / Metric | `piccolo-ax-desktop` (Piccolo-AX Workbench) | Raspberry Pi OS (Desktop / Full) |
| :---- | :---- | :---- |
| **Root Footprint & Boot Model** | **`< 42 MB` core overlay** on `< 512 MB` `toram` `EROFS`; opt-in boot mode with zero flash wear. | **Multi-gigabyte writable Debian rootfs** (`~4–10+ GB`) running continuously from SD/NVMe. |
| **CPU & RT Core Isolation** | Quarantined onto **Core 5** (`0x20`); completely absent during real-time console/audio boots. | Desktop compositor, browser, and background services share all 4 ARM cores with media/emulators. |
| **Browser & Office Stack** | Native C++ `Falkon`, `AbiWord`, `Gnumeric`, `Foliate`, `MuPDF`/`qpdfview`—fast startup in RAM. | Heavy Chromium/Firefox builds that quickly consume `1–2 GB+` RAM and trigger swap thrashing. |
| **Retro & Pixel Creation** | Purpose-built `GrafX2`, `LibreSprite`, `mtPaint`, `AzPainter`, `MilkyTracker`, `lhasa`, and `GHex` out of the box. | Requires manual package installation; lacks pre-integrated Amiga `.lha` and tracker tooling. |
| **Heavy Content Creation** | Opt-in `profiles/desktop-heavy.mk` (`GIMP`, `Krita`, `Shotcut`, `Kdenlive`, `HandBrake`, `KeePassXC`) backed by `i5-9600K` \+ `GTX 980 Ti` \+ `32 GB` RAM. | Limited by ARM SoC memory bandwidth and thermal throttling on multi-track video/RAW workflows. |
| **Audio Routing & Diagnostics** | Direct ALSA (`QASMixer`, `mhWaveEdit`) \+ live hardware telemetry (`CPU-X`, `nvtop`, `htop`). | `PipeWire`/`PulseAudio` mixing layer sits between applications and ALSA hardware by default. |

# Chapter 5: 16-Bit Home Computer, Tracker & Arcade Preservation Matrix

A centerpiece of The Silicon Preservation Society is faithful preservation of the **16-bit Commodore Amiga, Atari ST, and coin-op arcade era**—where custom co-processors, `50 Hz` PAL raster timing, and hardware DMA audio punish any host OS that introduces scheduler jitter.

| Classic Platform / Engine | Original Silicon & Defining Magic | `[AX]` Implementation & Hardware Isolation |
| :---- | :---- | :---- |
| **ProTracker / OctaMED / TCB Tracker** *(Amiga `.MOD` & ST `.SNDH` Audio)* | Hardware DMA sample playback with zero CPU audio overhead and unmistakable stereo separation | **`FLAC_AX` (`libopenmpt` \+ `libFLAC++`)** — Bit-perfect `S32_LE` (`44.1–384 kHz`) & `DoP` (`DSD64`/`128`) transport pinned to **Core 0** (`SCHED_FIFO 80`), with `--mod-stereo` separation control and zero ALSA resampling |
| **Commodore Amiga 500 / 1200 / CD32** *(OCS / ECS / AGA Custom Chipset)* | Copper co-processor raster effects, Blitter sprite pipelines, and Paula 4-channel DMA sound | **`Amiberry-AX`** — Linked against `ax-sdk` for `HugeTLB` fastmem, `2:3:1` core pinning (**Cores 2–4** CPU/chipset, **Core 1** scanout, **Core 0** Paula audio), and lockstep VBlank pacing |
| **Atari 520ST / 1040STE** *(68000 \+ YM2149 \+ STE DMA Sound)* | Crisp high-res productivity, built-in MIDI ports, and iconic demo-scene rasters | **`Hatari-AX` (`Generation-X-AX`)** — Cycle-accurate `68000` & `YM2149` execution isolated from OS interrupts across `2:3:1` cores with direct ALSA DAC routing |
| **Coin-Op Arcade & 16-Bit Crossover Hits** *(CPS-1/2/3, Neo Geo, Sega System 16\)* | The arcade boards Amiga and Atari ST owners dreamed of having at home | **`FinalBurn Neo` (`libretro-fbneo`)** — Integrated into `Piccolo-AX`'s EmulationStation wheel (`es_systems_fbneo.cfg`) on **Cores 2–4** (`0x1C`) and on `Piccolo-2go` **Core 2** (`0x04`) with `2 MB` HugePages |

# Chapter 6: Deep Dive — `Eden Espresso Wave [AX]` (Master `v1.34` / `v1.35` Ready)

## 6.1 Elevating Nintendo Switch Emulation to Professional Hardware Parity

**`Eden Espresso Wave [AX]`** (`git.eden-emu.dev/galaxywarriordad/Eden-Espresso-Wave-AX` and GitHub mirror) is a deterministic, hardware-locked fork of the Eden Nintendo Switch Emulator engineered specifically for **Intel Coffee Lake Refresh (`i5-9600K` / `i5-8600K`)**, **NVIDIA Maxwell/Pascal GPUs (`GTX 980 Ti` 6 GB)**, **Intel Quick Sync Video (`QSV`)**, and **bit-perfect exclusive USB DAC audio**.

Rather than treating Switch emulation as a generic desktop process competing with background Windows or Linux tasks, `Eden Espresso Wave [AX]` transforms the host PC into a focused, finely tuned gaming appliance carefully aligned with the system's physical silicon topology:

* **Three-Stage LLVM Profile-Guided Optimization (`PGO`):** Built with LLVM `clang-cl` (`-march=skylake`, `BMI2`), `ThinLTO`, and automated Windows batch orchestration (`Scripts/PGO_config.bat`, `PGO_stage0I.bat`, `PGO_stage1.bat`, `PGO_stage2.bat`), capturing hot branch/block execution profiles from real gameplay to optimize ARM64-to-x86\_64 JIT translation and GPU command submission.  
* **Elastic `2:3:1` Core & IRQ Partitioning:** Guest Switch CPU threads execute across isolated **Cores 2–4** (`0x1C`), Vulkan command submission and VBlank presentation stay locked to **Core 1** (`0x02`), exclusive USB DAC audio (`FlexASIO` on Windows / direct ALSA `SCHED_FIFO 80` on `Piccolo-AX`) runs on **Core 0** (`0x01`), and shader cache compression (`ZSTD`) \+ iGPU decode run on **Core 5** (`0x20`).  
* **Maxwell `5.50 GiB` VRAM Ceiling & BAR1 Shield:** Enforces proactive VRAM eviction at `5.00–5.50 GiB` on `6 GB` `GTX 980 Ti` cards and bounds `HOST_VISIBLE` staging buffers (`128–256 MiB`) so heavy Switch textures never spill into slow PCIe system RAM.  
* **Intel `QSV` / `VA-API` Hardware Video Offload:** Routes Switch `H.264`/`HEVC` cutscenes through integrated `Intel UHD Graphics 630` hardware decoders (`QSV` / `iHD` `dma_buf`), leaving CUDA cores dedicated strictly to 3D rendering.

## 6.2 `Eden Espresso Wave [AX]` vs. Standard Desktop Switch Emulation

| Architectural Dimension | `Eden Espresso Wave [AX]` | Standard Upstream / Untuned Switch Emulator |
| :---- | :---- | :---- |
| **Compiler & Binary Optimization** | LLVM `clang-cl` (`Skylake`/`BMI2`), `ThinLTO`, and 3-stage gameplay-trained **PGO**. | Generic `x86-64` MSVC/GCC builds without gameplay branch-profile feedback. |
| **CPU Thread & IRQ Topology** | Hardware-locked **`2:3:1`** isolation (**Core 0** audio, **Core 1** Vulkan, **Cores 2–4** JIT, **Core 5** QSV/I/O). | OS scheduler migrates JIT, audio, and shader threads across all cores dynamically. |
| **6 GB GPU VRAM Management** | Proactive **`5.50 GiB` VRAM ceiling** \+ `128–256 MiB` BAR1 pool prevents Maxwell memory thrashing. | Unbounded VRAM growth triggers driver-level PCIe RAM paging and severe frame spikes. |
| **Cutscene Video Decoding** | Offloaded to **Intel UHD 630 `QSV` / `VA-API`**, preserving discrete GPU shaders for 3D rendering. | Competes with 3D rendering queues on the discrete GPU or spikes CPU usage. |
| **Audio Latency & DAC Path** | Exclusive `FlexASIO` (Windows) / `SCHED_FIFO 80` ALSA `S32_LE` (Linux) with `/run/lock/ax-dac.lock`. | Shared OS audio mixer (`WASAPI` shared / `PulseAudio` / `PipeWire`) with resampling latency. |

# Chapter 7: System-by-System & Game-Level Emulation Breakdown Across `[AX]`

Every emulated console, computer, and arcade platform in the `[AX]` ecosystem is tuned for specific workload characteristics and representative benchmark titles:

## 1\. Nintendo Switch (`Eden-Espresso-Wave-AX` — `eden` / `Piccolo-AX`)

* **Engine & Topology:** ARM64 Dynarmic JIT on **Cores 2–4**, Vulkan renderer on **Core 1** (`5.50 GiB` VRAM cap), Intel `QSV` video decode on **Core 5**, exclusive DAC audio on **Core 0**.  
* **Representative Titles & Runtime Behavior:** Open-world and fast-action Switch titles—such as *The Legend of Zelda: Breath of the Wild* / *Tears of the Kingdom*, *Super Mario Odyssey*, *Mario Kart 8 Deluxe*, and *Metroid Dread*—benefit directly from 3-stage PGO reducing JIT dispatch overhead, Intel `QSV` preventing cutscene frame drops, and the `5.50 GiB` Maxwell ceiling eliminating texture-streaming stalls on the `GTX 980 Ti`.

## 2\. Nintendo Wii U (`Cemu-Latte-AX` — `Cemu-Latte-AX` / `Piccolo-AX`)

* **Engine & Topology:** PowerPC recompiler on **Cores 2–4**, `Latte` Vulkan pipeline on **Core 1** (`5.50 GiB` VRAM shield), Intel UHD 630 `VEBOX` video offload on **Core 5**, `SCHED_FIFO 80` USB DAC audio on **Core 0**.  
* **Representative Titles & Runtime Behavior:** Heavy Wii U exclusives—including *Xenoblade Chronicles X*, *The Legend of Zelda: The Wind Waker HD*, *Bayonetta 2*, and *Donkey Kong Country: Tropical Freeze*—maintain locked frame pacing without shader-compilation or audio-buffer crackles.

## 3\. Commodore Amiga 500 / 1200 / CD32 (`Amiberry-AX` — `Amiberry-AX` / `Piccolo-AX`)

* **Engine & Topology:** Cycle-exact `68000`/`68020`/`68030` \+ `OCS`/`ECS`/`AGA` custom chipset on **Cores 2–4**, `HugeTLB` fastmem via `ax-sdk`, lockstep `50 Hz` PAL VBlank scanout on **Core 1**, Paula 4-channel DMA audio on **Core 0**.  
* **Representative Titles & Runtime Behavior:** Raster-critical Amiga classics—*Shadow of the Beast* (multi-layer Copper parallax), *Turrican II* (Chris Hülsbeck TFMX/Paula audio \+ 50 Hz scrolling), *Sensible World of Soccer*, and *Alien Breed 3D* (`AGA`)—run with zero micro-stutter or audio pitch wobble.

## 4\. Atari 520ST / 1040STE (`Hatari-AX` — `Generation-X-AX` / `Piccolo-AX`)

* **Engine & Topology:** Cycle-accurate `68000` \+ `YM2149` PSG \+ STE DMA sound isolated via `ax::core` `2:3:1` pinning and direct ALSA DAC routing.  
* **Representative Titles & Runtime Behavior:** Sync-scrolling demo-scene productions and classic ST titles—*Dungeon Master*, *Starglider 2*, *Oids*, and *Lethal Xcess* (STE DMA audio)—preserve razor-sharp `50 Hz`/`60 Hz` border-removal rasters and crisp chiptune envelopes.

## 5\. Sega Dreamcast, NAOMI & Atomiswave (`Flycast-AX` — `Generation-X-AX` / `Piccolo-AX`)

* **Engine & Topology:** `SH-4` JIT \+ `PowerVR2` per-pixel/order-independent transparency rendering pinned via `ax_retro_bridge` (`2:3:1` desktop / `1:2:1` handheld).  
* **Representative Titles & Runtime Behavior:** Arcade-to-home powerhouses—*Soulcalibur*, *Ikaruga*, *Marvel vs. Capcom 2* (NAOMI), and *Metropolis Street Racer*—achieve deterministic `60 fps` frame delivery with zero input lag spikes.

## 6\. Nintendo 64 (`RMG-AX` — `Generation-X-AX` / `Piccolo-AX`)

* **Engine & Topology:** `Rosalie's Mupen GUI (RMG)` with `ParaLLEl-RDP` / `GLideN64` pinned to **Cores 2–4** and **Core 1**, sharing dynamic Qt6 libraries in `Piccolo-AX` RAM.  
* **Representative Titles & Runtime Behavior:** Timing-sensitive N64 titles—*F-Zero X* (locked `60 fps`), *Wave Race 64*, *Sin and Punishment*, and *Paper Mario*—run free from framebuffer readback hiccups.

## 7\. Sega Mega Drive / Genesis (`BlastEm-AX` — `Generation-X-AX` / `Piccolo-AX`)

* **Engine & Topology:** Cycle-accurate `68000` \+ `Z80` \+ `YM2612` FM synthesis isolated on **Cores 2–4** with **Core 0** `S32_LE` audio output.  
* **Representative Titles & Runtime Behavior:** Raster-split and fast-scrolling Mega Drive benchmarks—*Sonic the Hedgehog 2*, *Gunstar Heroes*, *Streets of Rage 2* (Yuzo Koshiro `YM2612` soundtrack), and *Contra: Hard Corps*—deliver sub-frame input response and authentic FM punch.

## 8\. Sega OutRun Arcade Engine (`Cannonball-AX` — `Generation-X-AX` / `Piccolo-AX`)

* **Engine & Topology:** Enhanced C++ `OutRun` arcade engine pinned via `ax::core` for locked `60 fps` widescreen sprite scaling and `S32_LE` PCM audio.  
* **Representative Titles & Runtime Behavior:** *OutRun* (`Magical Sound Shower`, `Passing Breeze`, `Splash Wave`) runs at a glass-smooth `60 Hz` with crisp analog steering/pedal response.

## 9\. Classic 3D PC & Space Simulation (`EDuke32-AX` & `Frontier-AX` — `Generation-X-AX` / `Piccolo-AX`)

* **Engine & Topology:** `EDuke32` (Build engine) and `Frontier: Elite II` (`glfrontier`) wrapped in `ax_retro_bridge` (`2:3:1` thread affinity and direct ALSA DAC audio).  
* **Representative Titles & Runtime Behavior:** *Duke Nukem 3D* (`Atomic Edition` / high-framerate `Polymost`/`Polymer`) and *Frontier: Elite II* (Newtonian orbital flight across the Milky Way) run cleanly inside the bare-metal appliance wheel without desktop background interference.

## 10\. Coin-Op Arcade Boards (`FinalBurn Neo` — `Piccolo-AX` & `Piccolo-2go`)

* **Engine & Topology:** `libretro-fbneo` on **Cores 2–4** (`0x1C`) in `Piccolo-AX` and on **Core 2** (`0x04`) with `ax_hugepage_shim.so` (`2 MB` HugePages) \+ `RGA2-Lite` `2x` integer scaling on `Piccolo-2go` (`RK3326`).  
* **Representative Titles & Runtime Behavior:** Capcom `CPS-1/2/3` (*Street Fighter III: 3rd Strike*, *Ghouls 'n Ghosts*, *Progear*), SNK `Neo Geo MVS` (*Metal Slug X*, *Garou: Mark of the Wolves*, *The King of Fighters '98*), and `Sega System 16` (*Shinobi*, *Golden Axe*) lock onto exact native refresh pacing (`59.63 Hz` / `60.00 Hz`) with zero audio crackles even on the `1 GB` `BATLEXP G350` and `R36S`.

# Chapter 8: Side-by-Side Comparison Matrix Across All Ecosystem Apps & Engines

| Application / Engine | Repository / Package | Target ISA & OS | Core & IRQ Topology | Memory, GPU & Hardware Offload | Primary Performance Advantage |
| :---- | :---- | :---- | :---- | :---- | :---- |
| **`flac_ax`** | `FLAC_AX (flac-ax)` | `x86_64 & aarch64 Linux` | **Core 0** (`SCHED_FIFO 80`), **Core 5** / **Core 3** preloader | `mlockall` (`WholeAlbum` `8 GiB` / `64 MiB` handheld), `S32_LE` & `DoP`, zero resampling | Eliminates storage bus interrupts and software sample-rate conversion for bit-exact DAC playback |
| **`ax_av_studio`** | `FLAC_AX` | `x86_64 Linux` | **Core 0** (`2.00 ms` RT), **Core 1** GL HUD, **Cores 2–4** DSP, **Core 5** MIDI | Preallocated `std::array` DSP (`AVX2+FMA`), `< 64 MB` VRAM HUD, `1024`\-slot SPSC MIDI | Hardware-synth key-to-ear latency (`2.00 ms`) with VBlank 60/120 Hz HUD and zero RT heap allocations |
| **`ax_video_upscaler`** | `FLAC_AX` | `x86_64 Linux` | **Core 0** PTS audio, **Core 1** GL shader, **Core 5** V4L2 capture | Lock-free `FrameSlot` seqlock, OpenGL 3.3 `GL_R8`\+`GL_RG8` `Y`/`UV` shader, `MS2130S` | Tear-free `720p`/`1080p`/`2K` retro HDMI & DVD scaling locked to the hardware DAC clock |
| **`ax_appliance_tui` & `ax_encode_2go`** | `FLAC_AX / Piccolo-AX` | `x86_64 Linux` | **Core 5** (`0x20` housekeeping) | Probes `/run/lock/ax-dac.lock` & `/proc/meminfo`; Intel `QSV`/`CUDA` `640x480` encoder | Zero CPU impact (`0%`) on RT audio and emulation cores during TUI navigation or batch transcoding |
| **`Eden-Espresso-Wave-AX`** | `eden / Piccolo-AX (eden-ax)` | `x86_64 Win10/11 & Linux` | Elastic `2:3:1` (**Cores 2–4** JIT, **Core 1** Vulkan, **Core 0** audio, **Core 5** QSV) | LLVM `clang-cl` ThinLTO \+ 3-stage PGO, Intel `QSV` offload, `5.50 GiB` Maxwell VRAM shield | Sustained Switch frame rates on `i5-9600K` \+ `GTX 980 Ti` without VRAM overflow or audio crackles |
| **`Cemu-Latte-AX`** | `Cemu-Latte-AX / Piccolo-AX` | `x86_64 Win10/11 & Linux` | `2:3:1` (**Cores 2–4** PPC JIT, **Core 1** Vulkan, **Core 0** audio, **Core 5** VEBOX) | PGO \+ `ax::core` at `v1_36_2`, Intel UHD 630 `VEBOX` video offload, `5.50 GiB` VRAM ceiling | Deterministic Wii U frame pacing and isolated USB DAC audio on legacy desktops |
| **`Amiberry-AX`** | `Amiberry-AX / Piccolo-AX` | `x86_64 Linux` | `2:3:1` (**Cores 2–4** CPU/chipset, **Core 1** scanout, **Core 0** Paula audio) | `HugeTLB` fastmem via `ax::core` at `v1_36_2`, `toram` EROFS staging | Cycle-consistent `50 Hz` PAL scrolling and jitter-free Amiga audio |
| **`Generation-X-AX`** | `Generation-X-AX / Piccolo-AX` | `x86_64 & aarch64 Linux` | Unified `ax::core` at `v1_36_2` `2:3:1` / `1:2:1` affinity across 7 engines | `ax_retro_bridge` fastmem, `/run/lock/ax-dac.lock` arbitration | Uniform low-latency execution across N64, Mega Drive, OutRun, Dreamcast, Atari ST, EDuke32, Frontier |
| **`FBNeo` & `mpv` (`Piccolo-2go`)** | `Piccolo-2go` | `aarch64 (RK3326)` | `1:2:1` (**Core 0** `SCHED_FIFO 80` audio, **Core 1** GPU/RGA, **Core 2** FBNeo, **Core 3** SD/ZRAM) | `ax_hugepage_shim.so` (`2 MB`), `RGA2-Lite` `2x` scaler, `1024 MB LZ4` ZRAM, `HiBy FC3` OTG | Native `640x480 @ 60.00 Hz` arcade timing and zero-resampling DAC audio on a `1 GB` handheld |
| **`piccolo-ax-desktop`** | `piccolo-ax-desktop / Piccolo-AX` | `x86_64 Linux` | Quarantined on **Core 5** (`0x20`) | `< 42 MB` (`< 35 MB` stripped) `LXQt`/`XFCE`, `Falkon`, pixel/binary utilities \+ optional heavy tier | Full desktop workstation capabilities without compromising real-time appliance modes |

# Chapter 9: Three Strategic Enhancements to Make `[AX]` a World-Class Legacy Hardware Platform

1. **Unified Automated Read-Only Self-Audit (`ax-doctor` Across `x86_64` and `aarch64`):** Expand `ax-sdk/tools/ax_doctor.sh` and `check_hardware.ps1` as read-only, on-demand pre-flight checks (without background polling or AI/ML inference) that verify `isolcpus`, `irqaffinity`, `SCHED_FIFO` privileges, `HugeTLB` / `ZRAM` / `toram` allocations, `/run/lock/ax-dac.lock` state, and `HiBy FC3` / `RK817` mixer gain staging in one pass.  
2. **Automated Reproducible Snapshot & Release Packaging:** Standardize a release packaging script that exports clean source archives (excluding local `CMakeFiles/` and `build/` directories), verifies pinned `v1_36_2` SDK tags across all consumer `CMakeLists.txt` files, and publishes SHA-256 checksums alongside CI build gates.  
3. **Expanded Drop-In Legacy Silicon Profiles (`hardware_profiles.json`):** Extend `ax-sdk/config/hardware_profiles.json` with pre-validated profiles for additional common legacy desktop CPUs (`4-core` Haswell/Skylake `1:2:1` variants, `8-core` Ryzen CCX topologies), `4 GB`–`8 GB` Maxwell/Pascal GPUs, and additional UAC 2.0 USB DACs so newcomers rescuing older PCs get deterministic tuning out of the box.
