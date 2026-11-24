# The Silicon Preservation Society (`[AX]`) vs. Raspberry Pi — Architectural Comparison & Roadmap

> *A tale of two philosophies: off-the-shelf single-board convenience versus unapologetic, bare-metal silicon mastery.*

Walk into almost any modern living room or tinkering desk looking for a compact media, retro-gaming, or appliance box and the **Raspberry Pi** is the default answer. It is cheap, cheerful, supported by an army of tutorials, and happy to juggle a desktop environment, background services, and a media player all at once.

**The Silicon Preservation Society (`[AX]`)** plays a very different game. Rather than buying new single-board computers or conceding capable mid-tier `x86_64` PCs and budget `RK3326` handhelds to e-waste, `[AX]` treats existing hardware as precision instruments—stripping away OS jitter, locking audio and frame pacing to dedicated CPU cores, and feeding external USB DACs bit-perfect streams without software resampling.

Where a general-purpose Raspberry Pi setup behaves like a Swiss Army knife running in the background, the `[AX]` stack behaves like a dedicated hi-fi transport and console appliance: **Core 0** is guarded for `SCHED_FIFO` audio and low-latency IRQs, **Core 1** handles display scanout, middle cores shoulder emulation or DSP synthesis, and **Core 5** (or **Core 3** on `RK3326`) quietly handles I/O, preloading, and encoding.

---

## Performance Differences: `[AX]` Ecosystem vs. Raspberry Pi

| Dimension | Silicon Preservation Society (`[AX]`) | Raspberry Pi (Typical General-Purpose Setup) |
| :--- | :--- | :--- |
| **Audio Timing & Bit-Perfection** | **Class-leading determinism:** `SCHED_FIFO` (priority 80) on dedicated Core 0, `S32_LE` / `DoP` (`DSD64` / `DSD128`) bypasses desktop mixers/resampling, paired with `mlockall` RAM locking and external USB-C DACs (such as the HiBy FC3). | **Good for everyday listening, prone to contention:** Standard Linux audio stacks and shared CPU cores can experience occasional scheduling jitter under heavy I/O or background tasks unless carefully tuned. |
| **Frame Pacing & Emulation Headroom** | **Purpose-built core partitioning:** `2:3:1` isolation on 6-core `x86_64` (`i5-9600K` / `i5-8600K`) with `HugeTLB` fastmem, VRAM guardrails (`5.50 GiB` ceiling), and `1:2:1` isolation on `RK3326` handhelds (`< 140 MB` `zstd` SquashFS). | **Broad compatibility, shared resources:** Great for classic 8-bit/16-bit/arcade and light 3D workloads, but threads compete across cores without strict appliance-level isolation by default. |
| **System Footprint & Boot Behavior** | **Lean, RAM-resident appliances:** `< 300 MB` `toram` EROFS (`Piccolo-AX` on `Linux 6.12-rt`) and `< 42 MB` quarantined X11 desktop (`piccolo-ax-desktop`) minimize disk thrash and background noise. | **Full-featured general OS:** Easy to expand with packages and desktop apps, at the cost of more background daemons and higher memory/storage overhead. |
| **Hardware Longevity & Value** | **Anti-obsolescence by design:** Extends the life of existing PCs, GPUs, and handhelds rather than requiring new SBC purchases. | **Purpose-built new hardware:** Low entry cost for new projects, though still subject to board-level I/O and GPU bottlenecks on heavier tasks. |

> **The Verdict:** If you want plug-and-play simplicity and broad general-purpose tinkering out of the box, the **Raspberry Pi** remains an easy recommendation. If your priority is **uncompromising clock stability, zero-jitter audio, deterministic frame pacing, and keeping proven silicon out of landfills**, **The Silicon Preservation Society** is in a league of its own.

---

## Three Things That Would Make `[AX]` a World-Class Ecosystem for Old Hardware

1. **Automated Hardware Profile & Kernel Isolation Self-Audit (`ax-doctor`):** Expand automated startup and diagnostic checks across `x86_64` and `aarch64` targets so users can verify in one glance that `isolcpus`, `irqaffinity`, `SCHED_FIFO` permissions, `HugeTLB` / `ZRAM` limits, and USB DAC mixer states are active—turning complex bare-metal tuning into foolproof verification.
2. **One-Command Reproducible Appliance & SDK Snapshots:** Standardize scripted release/snapshot packaging (clean source archives without build artifacts, pinned `v1_36_2` SDK tags, and verified checksums) so every appliance image and emulator build is 100% reproducible across machines and mirrors.
3. **Plug-and-Play Hardware Profiles for Common Legacy Rigs:** Ship curated drop-in profiles for popular legacy CPUs, GPUs, USB DACs, and retro handhelds beyond primary reference rigs—complete with safe conservative fallbacks and clear power/thermal guardrails—so anyone rescuing older gear gets near-instant, rock-solid results.
