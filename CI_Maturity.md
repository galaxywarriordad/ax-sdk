markdown
# 🛡️ `ax-sdk` Continuous Integration & SDK Maturity Roadmap **(`CI_MATURITY.md`)**

**Programme Banner:** [The Silicon Preservation Society](https://ko-fi.com)  
    
**Target Repository:** `galaxywarriordad/ax-sdk` (`ax::core`)  
    
## 🧭 Scope:** Levels 1–5 CI Gates (`v1.36.0` → `v1.39+`)
    
This document outlines a 5-level continuous integration and SDK maturity roadmap (v1_36_1 through v1.39+) drawing inspiration from console NDKs, AAA middleware, and kernel engineering.
• Level 1 (Hygiene & Telemetry): Multi-compiler -Werror matrix (Clang, GCC, MSVC/clang-cl), target-scoped CMake isolation, cache-line-padded shared-memory telemetry (ax::telemetry), and single-header amalgamation (ax_sdk.hpp).
• Level 2 (Correctness): Injectable CPU/topology mocks and ThreadSanitizer (TSan) / AddressSanitizer (ASan) / UndefinedBehaviorSanitizer (UBSan) concurrency sweeps.
• Level 3 (Determinism): Zero-heap-allocation hot-path assertions and profiler timeline hooks (Tracy/PIX).
• Level 4 (Ecosystem): Semantic versioning guardrails and automated downstream canary builds across Eden, Cemu, and FLAC_AX.
• Level 5 (Silicon HIL Lab): Automated bare-metal Hardware-in-the-Loop replay validation on reference hardware (i5-9600K + GTX 980 Ti) enforcing zero audio underruns and strict VRAM/image size budgets.
• The complete markdown file including the reference GitHub Actions CI workflow blueprint is available in reference.
    
---

## 🧭 Introduction: Why Console-Grade CI Matters for Hardware Preservation

### Where We Looked for Inspiration
Most open-source emulation libraries evolve organically around whatever hardware their primary maintainer owns, relying on ad-hoc manual testing and basic "does it compile?" checks. To make **`ax-sdk`** a dependable foundation across multiple emulator forks (`Eden`, `Cemu`, `RPCS3`, `shadPS4`, `Xenia`) and bare-metal Linux appliances (`Piccolo-AX`), we looked beyond typical desktop app workflows toward three proven engineering domains:

* **Console Platform NDKs (PlayStation, Nintendo, and Xbox Native Development Kits):** Console SDKs treat CPU core topology, memory page tables, DMA transfer queues, and audio ring buffers as strict hardware contracts—enforcing zero-allocation hot paths, clean header isolation, and backward-compatible versioning so game engines never break on SDK updates.
* **AAA Game-Engine Middleware (RAD Telemetry, Oodle, Havok, and FMOD):** Commercial middleware ships into dozens of host engines with wildly different compiler flags and build systems. These SDKs survive by enforcing strict warning hygiene (`-Werror`), exposing cache-line-aligned telemetry counters without log-scraping overhead, and providing lightweight trace hooks (`Tracy` / `PIX` / `RenderDoc`).
* **Low-Level Linux Graphics & Kernel Engineering (Valve `gamescope`, `DXVK`, and `PREEMPT_RT`):** Modern frame-pacing and memory-guard layers rely on VBLANK slack windows, deterministic thread priority ceilings, and automated sanitizer/hardware-in-the-loop regression gates to catch micro-stutters and race conditions before releases ship.

---

### What This Roadmap Brings to `ax-sdk`
Adopting a structured five-level maturity progression elevates `ax-sdk` from a collection of hardware-specific optimizations into a **self-verifying, production-grade C++20 systems runtime**:

* **Zero-Friction Integration for Downstream Forks:** Target-scoped CMake features, standalone header compilation checks, and single-header amalgamation (`ax_sdk.hpp`) guarantee that linking `ax::core` never pollutes a host emulator's global compiler flags or macros.
* **Mathematically Verified Concurrency & Topology:** ThreadSanitizer (`TSan`) stress sweeps and injectable silicon mocks prove that lock-free SPSC audio rings (`ax::audio`) and `2:3:1` + L3 CCX affinity masks (`ax::topology`) behave deterministically across 4-core, 6-core, and multi-CCX Ryzen/Intel CPUs—without requiring every contributor to own physical test hardware.
* **Predictable Console-Class Frame Pacing:** Hot-path zero-allocation assertions and structured `ax::Init()` fallback diagnostics ensure real-time audio (`Core 0`) and Vulkan submission (`Core 1`) threads never stall on heap locks, missing HugePages, or restricted MSR permissions.
* **Empirical Regression Protection:** Automated downstream canary builds and bare-metal Hardware-in-the-Loop (HIL) replay gates verify every optimization against our unmodified **Stage A** (`Clang 23+` ThinLTO + 3-Phase PGO) control baseline.

---

### How This Document Is Used
This roadmap serves as both an **engineering execution checklist** and a **contributor contract** across The Silicon Preservation Society ecosystem:

1. **For `ax-sdk` Maintainers & Contributors:** Each maturity level (`Level 1` through `Level 5`) maps directly to a concrete release milestone (`v1.36.0` → `v1.39+`), defining the exact GitHub Actions workflows, `CTest` suites, and sanitizer gates required before bumping semantic versions.
2. **For Downstream Emulator Fork Authors (`Eden`, `Cemu`, `RPCS3`, `shadPS4`, `Xenia`):** Downstream teams can check `ax-sdk`'s active maturity badge to verify ABI/API stability guarantees, inspect supported `ax::Telemetry` OSD hooks, and rely on automated canary builds to catch integration issues before updating `FetchContent` tags.
3. **For `Piccolo-AX` Appliance & Bare-Metal Testers:** Level 5 defines the objective pass/fail hardware gates (`0` audio underruns, `< 5.50 GiB` Maxwell VRAM ceiling, `0` `LOC` timer ticks on isolated cores, and `< 300 MB` `toram` EROFS footprint) required to certify a release for bare-metal deployment.
---

## 📈 The Five-Level CI & SDK Maturity Roadmap

### Level 1 — Toolchain & Header Hygiene (`v1.36.0`)
*Console SDK Equivalent: Clean multi-compiler headers & zero-warning compliance.*

* **Multi-Compiler CI Matrix (`.github/workflows/ci.yml`):** Gate every push and pull request across Windows (`clang-cl 23+` and `MSVC 19.4x+`) and Linux (`Clang 19+` and `GCC 14+`) with `/W4 /WX` and `-Wall -Wextra -Wpedantic -Werror` enabled privately on `ax_core`.
* **Target-Scoped CMake Isolation:** Enforce `target_compile_features(ax_core ... cxx_std_20)`, `Threads::Threads` linkage, and `PRIVATE` Win32 header macros (`NOMINMAX`, `WIN32_LEAN_AND_MEAN`) so `ax-sdk` never mutates parent emulator compiler flags.
* **Standalone Header Self-Containment Gate:** Compile an isolated `.cpp` translation unit per public header (`#include <ax/config.hpp>`, `telemetry.hpp`, `topology.hpp`, `memory.hpp`, `vram.hpp`, `igpu.hpp`, `audio.hpp`) in CI to verify zero missing transitive includes.
* **Single-Header Amalgamation Artifact (`ax_sdk.hpp`):** Automatically generate and compile-test the bundled single-header `ax_sdk.hpp` on every tagged release for zero-CMake consumers.

### Level 2 — Hardware-Mock Unit & Sanitizer Gates (`v1.36.5` – `v1.37.0`)
*Console SDK Equivalent: Deterministic hardware abstraction verification without devkit silicon.*

* **Injectable Silicon Topology Mocks (`CTest`):** Decouple `CPUID`, `GetLogicalProcessorInformationEx`, `sysfs`, and `VkPhysicalDevice` queries behind mock descriptors so headless CI runners verify `2:3:1` + L3 CCX core masks across simulated 4C, 6C (`i5-9600K`), 8C, and 16C dual-CCX Ryzen layouts.
* **ThreadSanitizer (`TSan`) Concurrency Stress Sweeps:** Run `LockFreeStereoRing` (`ax::audio`) and `VkVramShield` (`ax::vram`) under Linux `TSan` with millions of concurrent SPSC push/pop wrap-arounds to prove lock-free memory ordering (`std::memory_order_acquire`/`release`) is race-free.
* **Address & Undefined Behavior Sanitizers (`ASan` + `UBSan`):** Execute all `CTest` suites under `ASan`/`UBSan` to catch alignment violations, out-of-bounds ring indexing, or signed overflow in BMI1 bit-extraction helpers.

### Level 3 — Studio Frame-Budget & Allocation Contracts (`v1.37.5`)
*Console SDK Equivalent: Zero-allocation hot paths & strict CPU/GPU frame-time budgets.*

* **Zero-Heap-Allocation Hot-Path Assertions:** Instrument `ax::audio` callbacks, `ax::vram` VBLANK reap checks, and `ax::memory` page translations in debug/CI builds to assert `0` `malloc`/`new` calls or OS mutex locks on real-time threads (`Core 0` / `Core 1`).
* **Structured `ax::Telemetry` & Profiler Hooks (RAD Telemetry / `Tracy` Style):** Expose cache-line-padded (`alignas(64)`) atomic counters (`audio_underruns`, `vram_reaps`, `bar1_stalls`, `igpu_offloads`) via `ax::GetTelemetrySnapshot()` plus optional user-supplied trace callbacks (`on_zone_begin` / `on_zone_end`) for direct Tracy, RenderDoc, or PIX timeline integration.
* **Deterministic Lifecycle & Fallback Contract (`ax::Init` / `ax::Status`):** Enforce explicit initialization returning structured diagnostic status codes (`Status::Ok`, `Status::FallbackNoHugePages`, `Status::FallbackSingleGpu`, `Status::FallbackNoMsrAccess`) with zero silent global side effects.

### Level 4 — ABI/API Stability & Downstream Canary Builds (`v1.38.0`)
*Console SDK Equivalent: NDK versioned headers, deprecation policy, and downstream title build verification.*

* **Semantic Versioning & API Compatibility Guard:** Enforce strict `AX_VERSION_MAJOR/MINOR/PATCH` guarantees so minor releases (`1.36.x` / `1.37.x`) never break source compatibility for pinned downstream emulator forks.
* **Automated Downstream Canary Matrix:** Trigger automated build-and-link checks against **`Eden-Espresso-Wave-AX`** and **`Cemu-Latte-AX`** (and later `RPCS3-AX` / `shadPS4-AX`) whenever an `ax-sdk` pull request opens—catching integration breaks before a release tag is cut.

### Level 5 — Hardware-in-the-Loop (HIL) Silicon Regression Lab (`v1.39+` / `Piccolo-AX`)
*Console SDK Equivalent: Automated overnight devkit farm running deterministic gameplay replays.*

* **Self-Hosted HIL Runner (`i5-9600K` + `GTX 980 Ti` + `UHD 630` + `HiBy FC3`):** Run an automated 60-second deterministic replay harness (`benchmark_frametimes.ps1` / `Piccolo-AX` CI) comparing **Stage A** (Vanilla ThinLTO+PGO) vs. **Stage B** (`ax-sdk` active), failing the pipeline if:
  * `audio_underruns > 0`
  * $P_{99.9}$ frame-time jitter regresses beyond threshold
  * Resident VRAM exceeds the `5.50 GiB` (`5.35 GiB` LSFG) Maxwell ceiling
  * `Piccolo-AX` `rootfs.erofs` exceeds `< 300 MB` or logs `> 0` `LOC` timer ticks on isolated Cores `0–4`

---

## 📊 Maturity Progression Summary

| Maturity Level | Target Version | Core Gate Added | Key Studio / Console Principle |
| :--- | :--- | :--- | :--- |
| **Level 1: Hygiene** | `v1.36.0` | Multi-compiler `-Werror`, standalone header compile, `ax_sdk.hpp` | Never break consumer build flags |
| **Level 2: Correctness** | `v1.36.5` – `v1.37.0` | Hardware-mock `CTest` + `TSan`/`ASan`/`UBSan` concurrency sweeps | Prove lock-free & topology math without devkits |
| **Level 3: Determinism** | `v1.37.5` | Hot-path `0-alloc` assertions, `ax::Telemetry` + Tracy/PIX hooks | Console-grade real-time frame budgets |
| **Level 4: Ecosystem** | `v1.38.0` | SemVer API guard + automated `Eden`/`Cemu` canary builds | Protect downstream forks from breaking changes |
| **Level 5: Silicon HIL** | `v1.39+` | Bare-metal `9600K + 980 Ti` replay gate (`0` xruns, `< 5.50 GiB` VRAM) | Empirical proof against Stage A PGO control |

---

## ⚙️ Reference GitHub Actions CI Blueprint (`.github/workflows/ci.yml`)

```yaml
name: ax-sdk CI (Levels 1-2)

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build-and-test:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: ubuntu-latest
            cc: clang
            cxx: clang++
            sanitizers: "-fsanitize=thread"
          - os: ubuntu-latest
            cc: gcc
            cxx: g++
            sanitizers: "-fsanitize=address,undefined"
          - os: windows-latest
            cc: clang-cl
            cxx: clang-cl
            sanitizers: ""

    runs-on: \${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - name: Configure CMake
        run: >
          cmake -B build -G Ninja
          -DCMAKE_C_COMPILER=\${{ matrix.cc }}
          -DCMAKE_CXX_COMPILER=\${{ matrix.cxx }}
          -DAX_SDK_ENABLE_WARNINGS=ON
          -DAX_SDK_BUILD_TESTS=ON
          -DCMAKE_CXX_FLAGS="\${{ matrix.sanitizers }}"

      - name: Build & Run CTest
        run: |
          cmake --build build --parallel
          ctest --test-dir build --output-on-failure
```
