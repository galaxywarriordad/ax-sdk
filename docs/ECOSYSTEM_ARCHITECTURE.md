# The Silicon Preservation Society — Ecosystem Architecture & Repository Topology

> **Canonical Reference:** This document is maintained in [`ax-sdk`](https://github.com/galaxywarriordad/ax-sdk) as the single source of truth for cross-repository architecture across The Silicon Preservation Society ecosystem.

## 1. Architectural Thesis & Design Principles

The Silicon Preservation Society ecosystem is engineered around **deterministic hardware preservation**: keeping proven x86_64 and ARM64 computing hardware out of landfills by eliminating OS background jitter, audio resampling, and uncoordinated thread scheduling.

All nine repositories enforce a strict **Policy-in-SDK / Adapter-in-Consumer** boundary across three runtime tiers:

1. **Shared Runtime Core (`ax-sdk`):** Dependency-free C++20 primitives for physical CPU topology discovery, core/IRQ isolation masks, engine configuration parsing, HugeTLB fastmem, VRAM guardrails, and hardware video offloading.
2. **Bare-Metal OS & Workbench Layer (`Piccolo-AX`, `Piccolo-2go`, `piccolo-ax-desktop`):** Immutable compressed root filesystems (`EROFS` / `SquashFS` running `toram`) with kernel-level CPU partitioning (`isolcpus`, `nohz_full`, `rcu_nocbs`, `irqaffinity`), real-time scheduling (`6.12-rt`), and isolated user-space modes.
3. **Media & Emulation Consumer Layer (`FLAC_AX`, `Eden-Espresso-Wave-AX`, `Cemu-Latte-AX`, `Amiberry-AX`, `Generation-X-AX`):** Domain-specific C++20 workloads that consume `ax-sdk` policies and OS isolation guarantees without duplicating platform scheduling logic.
