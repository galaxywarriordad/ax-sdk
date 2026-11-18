# 🤝 Contributing to ax-sdk

Thank you for your interest in **`ax-sdk`** (`libax_core`)! Because this library provides the shared C++20 hardware-abstraction primitives for **Eden Espresso Wave `[AX]`**, **Piccolo `[AX]`**, and **Cemu Latte `[AX]`**, all contributions must maintain zero external third-party dependencies and strict cross-platform ABI hygiene.

---

## ⚖️ Legal & Clean-Room Policy

* **Clean-Room Implementations Only:** `ax-sdk` is licensed under the **MIT License** so it can be linked cleanly into GPL-3.0, MPL-2.0, and MIT emulator codebases. Do not submit code copied from GPL-only repositories or proprietary SDKs.
* **Zero Proprietary Assets:** Never attach or commit copyrighted console firmware, cryptographic keys, game dumps, or proprietary binaries (`Lossless.dll`).

---

## 🛠️ Engineering & Code Standards

All code contributions must compile cleanly with zero warnings (`-Wall -Wextra -Werror`) on **LLVM `clang-cl` 23+** (Windows `/MD`) and **GCC 13+ / Clang 18+** (`PREEMPT_RT` Linux):

1. **Zero External Runtime Dependencies:**
   * Rely strictly on the C++20 standard library, compiler intrinsics (`<immintrin.h>`), and native OS headers (`<windows.h>` on Win32; `<pthread.h>`, `<sched.h>`, `<sys/mman.h>` on Linux).
2. **Win32 Header & Macro Hygiene:**
   * Always define `NOMINMAX` and `WIN32_LEAN_AND_MEAN` before including `<windows.h>`, and guard against Win32 macro collisions (`#ifdef CreateEvent` / `#undef CreateEvent`).
3. **Real-Time & Lock-Free Safety:**
   * Never allocate heap memory, acquire blocking mutexes, or perform file I/O inside audio ring (`ax::audio`) or hot fastmem (`ax::memory`) paths.
   * Use `std::memory_order_seq_cst` and explicit instruction cache invalidation (`FlushInstructionCache` / `__builtin___clear_cache`) for cross-core executable code updates.

---

## 🐛 Submitting Issues & Pull Requests

When opening an issue on **`galaxywarriordad/ax-sdk`**, please include:

1. **Target Consumer & OS:** Which emulator (`Eden-Espresso-Wave-AX`, `Piccolo-AX`, or `Cemu-Latte-AX`) and OS/kernel (`Windows 10/11` or `Linux 6.12-rt`) you are testing against.
2. **Host Silicon Topology:** CPU model (physical/logical core count, SMT/CCX layout) and GPU configuration.
3. **Compiler & Build Flags:** Compiler version (`clang-cl` / `gcc`) and CMake configuration flags.
