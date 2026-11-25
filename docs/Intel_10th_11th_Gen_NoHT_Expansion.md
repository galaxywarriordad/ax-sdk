# Deterministic Silicon: Building a Hardware-Locked Appliance From Capable PCs

If you have never heard of **The Silicon Preservation Society** (\[AX\]), the premise starts with a simple observation: modern computers and pocket handhelds rarely feel sluggish because their silicon has worn out—they feel sluggish because general-purpose operating systems, background tasks, software audio mixers, and scheduler contention constantly interrupt real-time work. A CPU core that can execute billions of instructions per second still stumbles on a 2.00 ms audio period or a 16.67 ms 60 Hz frame deadline if a background service, storage interrupt, or sibling thread barges into the pipeline at the wrong microsecond.

The \[AX\] ecosystem is a bare-metal, deterministic engineering stack—led by ax-sdk (ax::core) and the Piccolo-AX Linux appliance—designed to give bit-perfect audio, low-latency synthesis, retro console/DVD upscaling, and console/retro emulation dedicated hardware resources instead of fighting a busy desktop OS. It was **originally designed around 8th- and 9th-gen Intel desktop processors** (such as the i5-8600K and i5-9600K with 6 physical cores, zero Hyper-Threading, and an active on-die UHD Graphics 630 iGPU), where physical core counts, interrupt routing, and memory ceilings can be treated as predictable constants. On those non-SMT chips, there is no ambiguity between a physical core and a logical companion thread: six physical cores map directly to six logical CPUs, making strict core isolation, IRQ steering, and latency measurement straightforward.

---

## Why Hyper-Threading Has to Go Before 10th- and 11th-Gen Intel Can Shine

In the \[AX\] stack, determinism is not an optional luxury—it is the foundation that lets a preserved PC run bit-perfect S32\_LE / DoP audio, 2.00 ms live synthesis, 240p/480i retro capture, and locked-frame emulation without OS scheduler jitter. On 8th- and 9th-gen non-SMT chips like the i5-8600K and i5-9600K, each logical core maps one-to-one to a physical execution core (6C / 6T), giving a clean 2:3:1 split across audio/IRQ (Core 0), rendering/VRAM guard (Core 1), JIT/DSP (Cores 2–4), and housekeeping/I/O (Core 5).

When you step up to **10th-gen Comet Lake** and **11th-gen Rocket Lake** on LGA1200, Hyper-Threading is enabled by default across the Core i5, i7, and i9 lineup. Leaving Hyper-Threading on lets two logical threads share a physical core's execution pipelines, caches, and reorder buffers—introducing the exact execution-unit contention and "instruction flicking" that undermine deterministic frame and audio pacing. Even when a scheduler tries to be clever, sharing a physical core's execution resources means one thread's branch mispredictions, cache pressure, or port contention can ripple into a sibling thread doing latency-sensitive work.

Turning **Hyper-Threading off in firmware** restores single-thread-per-physical-core clarity (6C / 6T on i5 models, 8C / 8T on i7/i9 models, and 10C / 10T on the i9-10900K) while keeping your existing ax-sdk topology and hardware-preservation philosophy intact. Instead of treating logical thread count as a bigger number on paper, the \[AX\] approach treats **uncontended physical execution cores** as the scarce, high-value resource.

---

## Expanding the LGA1200 Preservation Lineup (No-HT \+ Active iGPU)

Two practical rules govern which 10th- and 11th-gen Intel CPUs make sense to bring into ax-sdk and Piccolo-AX: **disable Hyper-Threading in BIOS/UEFI**, and **require an active on-die Intel iGPU** (skipping \-F / \-KF SKUs) so hardware video decode and offload paths remain available. Disabling Hyper-Threading does not erase the generational upgrades these chips bring—it simply removes sibling-thread interference so higher clocks, newer memory/PCIe support, and integrated graphics translate into cleaner latency rather than extra contention.

### 1\. 10th-Gen Comet Lake (14nm, Ring Bus, UHD Graphics 630\)

* **6-Core i5 Drop-Ins (i5-10400, i5-10500, i5-10600K — 6C / 6T with HT Off):** Matches your i5-8600K and i5-9600K physical core count (12 MB L3 cache, UHD Graphics 630), allowing your exact 2:3:1 bitmasks (0x01, 0x02, 0x1C, 0x20) to carry over cleanly while adding newer LGA1200 platform availability (i5-10600K adds unlocked clocks and higher stock turbo headroom). For builders standardizing on a 2:3:1 core split across Piccolo-AX and Windows/Linux consumers, these are the most direct continuation of the original 6-core non-SMT blueprint.  
* **8-Core i7 Step-Ups (i7-10700, i7-10700K — 8C / 8T with HT Off):** Steps up to **8 physical cores**, 16 MB L3 cache, and UHD Graphics 630; with HT off, Cores 0–5 can preserve the familiar 2:3:1 core split while Cores 6–7 give extra physical breathing room for background I/O, preloading, encoding, or housekeeping (0x01 audio, 0x02 render, 0x3C 4 JIT/DSP cores, 0xC0 2 aux/housekeeping cores). That extra pair of physical cores is especially helpful when background file staging, RAM preloading, or capture/transcode tasks run alongside heavy emulation or synth work.  
* **10-Core Flagship (i9-10900K — 10C / 10T with HT Off):** Offers 10 physical cores, 20 MB L3 cache, and UHD Graphics 630, best reserved for higher-power/better-cooled LGA1200 builds (125W rated TDP) where extra physical cores are wanted without SMT. Because ten physical cores draw noticeably more power under sustained load than six, motherboard VRM quality and sustained cooling matter just as much as core bitmasks.

### 2\. 11th-Gen Rocket Lake (14nm, Cypress Cove IPC, PCIe 4.0, UHD Graphics 750\)

* **6-Core i5 (i5-11400, i5-11600K — 6C / 6T with HT Off):** Brings Cypress Cove IPC gains, 12 MB L3 cache, **Intel UHD Graphics 750** (32 EUs), and PCIe 4.0 support on compatible 500-series boards into a 6-core no-HT 2:3:1 footprint. Even with Hyper-Threading disabled, higher per-clock instructions-per-cycle helps single-threaded and latency-critical emulation/DSP loops complete faster inside each scheduling window.  
* **8-Core i7 & i9 (i7-11700, i7-11700K, i9-11900K — 8C / 8T with HT Off):** The strongest no-HT expansion target on LGA1200—combining **8 physical cores**, 16 MB L3 cache, UHD Graphics 750 (32 EUs), and higher single/all-core clocks, making it a natural fit for heavier emulation or offload experiments (such as testing shadPS4-AX before backporting to the i5-9600K) while keeping physical cores strictly isolated. Having eight physical cores with HT off gives you room to test heavier workloads without letting background tasks crowd out Core 0 audio or Core 1 rendering.

---

## Three Practical Features That Keep a Multi-Mode Appliance Honest

Expanding from the original 8th- and 9th-gen Intel baseline (i5-8600K and i5-9600K) into 10th- and 11th-gen no-Hyper-Threading processors only pays off if the surrounding system enforces discipline at boot, runtime, and mode switches:

* **Pre-Flight Firmware & iGPU Guard (ax\_doctor):** Because 10th- and 11th-gen desktop chips ship with Hyper-Threading enabled by default—and \-F / \-KF variants omit on-die graphics—a startup hardware check verifies physical-core isolation (flagging active SMT/HT siblings that would cause instruction contention) and confirms active iGPU offload availability before a workload starts.  
* **Live Shared-Memory Telemetry (ax-top & ax::telemetry):** Rather than guessing at audio jitter or memory pressure, shared-memory counters (Local\\AX\_Telemetry / /dev/shm/ax\_telemetry) and a lightweight 10 Hz monitor (ax-top) expose audio callback timing/underruns, VRAM headroom against the 5.00 GiB prune / 5.50 GiB cap, and iGPU offload activity—turning tuning into a repeatable measurement loop.  
* **Seamless Mode Handoff & DAC Locking (ax-mode-cli & ScopedDacLock):** Moving between bit-perfect music playback, a 2.00 ms synth or CLAP/LV2 session, 240p/480i retro or DVD upscaling, and emulation requires clean resource transitions. Exclusive DAC arbitration (/run/lock/ax-dac.lock via ScopedDacLock) and deterministic mode switching (ax-mode-cli) ensure one USB-C DAC and one preserved Intel box can serve every role on the desk without pops, lock contention, or lingering background noise.

---

## How They Fit Into ax-sdk & Your Six-Week Architecture

One of the biggest payoffs of how this ecosystem was built over roughly six weeks—starting with a two-to-three-week sprint on deterministic Eden, extracting the shared topology and runtime rules into ax-sdk, standing up Piccolo-AX and Piccolo-2go, and wiring the open-source audio, synth, upscaling, and emulation apps onto the stack—is that hardware expansion stays centralized rather than splintering across forks. Because ax-sdk centralizes topology policy (ax::topology) while keeping consumers (Eden-Espresso-Wave-AX, Cemu-Latte-AX, Generation-X-AX, FLAC\_AX, Piccolo-AX) as thin adapters, expanding to 10th- and 11th-gen no-HT profiles does not require rewriting emulator cores:

* **Preserve 2:3:1 on 6-Core No-HT (i5-10400–11600K):** Keep 0x01 / 0x02 / 0x1C / 0x20 on Cores 0–5 so existing timing, audio, and housekeeping assumptions remain intact across FLAC\_AX, ax\_av\_studio, ax\_video\_upscaler, and the emulator adapters.  
* **Define Explicit 8-Core No-HT Masks (i7-10700–11700K, i9-11900K):** Document and profile 8C / 8T (HT Off) explicitly (0x01 audio, 0x02 render, 0x3C / 0xC0 split across remaining physical cores) in SILICON\_TOPOLOGY\_PLAN\_v2.md and config/hardware\_profiles.json so 8-core no-HT systems never rely on SMT-skip bitmasks or accidentally dilute Cores 0–4.  
* **Enforce Power & Thermal Limits:** On higher-wattage K\-series LGA1200 chips, lock steady power/thermal behavior so frequency drops never create the same jitter as an OS scheduler interrupt. A CPU that boosts to 5.0 GHz for ten seconds and then throttles down under heat or power limits introduces clock variance that is just as disruptive to real-time pacing as an unpinned background thread.

By pairing **Hyper-Threading disabled in BIOS** with **active Intel integrated graphics** and **explicit physical-core profiles in ax-sdk**, 10th- and 11th-gen LGA1200 processors extend the life of capable desktop hardware without compromising the deterministic discipline that made the original 8th- and 9th-gen \[AX\] baseline work.
