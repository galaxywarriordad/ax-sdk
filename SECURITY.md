# 🛡️ Security Policy & Host System Privilege Notes

## ⚖️ Clean-Room & Legal Policy

**`ax-sdk`** is a clean-room, open-source C++20 hardware-abstraction library licensed under the **MIT License**.

* **Zero Proprietary Files:** This repository contains no copyrighted console firmware, cryptographic keys, game software, or proprietary third-party binaries.
* **Report Rejections:** Any issue or pull request referencing or attaching copyrighted console files or commercial binaries will be closed and removed immediately.

---

## 🔒 Host System Privileges & OS Interfaces

`ax-sdk` wraps standard, documented operating-system APIs on Windows 10/11 and Linux (`PREEMPT_RT`) without installing kernel drivers:

| Mechanism | Platform & API | Privilege Required | Security & Stability Impact |
| :--- | :--- | :--- | :--- |
| **Large / Huge Pages** (`ax::memory`) | Win32 `VirtualAlloc` (`MEM_LARGE_PAGES`) / Linux `mmap` (`MAP_HUGETLB`) | Windows: `SeLockMemoryPrivilege` (*Lock pages in memory*) · Linux: `vm.nr_hugepages` | Reduces DTLB misses for guest fastmem reserves. Automatically falls back to standard `4 KB` pages if unavailable. |
| **Thread Affinity & Real-Time Scheduling** (`ax::topology`) | Win32 `SetThreadAffinityMask` + MMCSS `Pro Audio` / Linux `sched_setaffinity` + `SCHED_FIFO` | Windows: Standard user · Linux: `CAP_SYS_NICE` (or `cgroups v2` slice) | Pins real-time audio, GPU submission, and JIT worker threads to isolated physical cores. |
| **Shared-Memory Telemetry** (`ax::telemetry`) | Win32 `Local\AX_Telemetry` / Linux `/dev/shm/ax_telemetry` | Standard user | Exposes lock-free atomic performance counters across processes without elevated privileges or disk I/O. |
| **Hardware MSI IRQ Steering** (`tools/set_irq_affinity.ps1`) | Windows PnP Registry (`Affinity Policy`) | **Administrator** (one-time registry write) | Writes `DevicePolicy = 4` and `AssignmentSetOverride` masks (`0x01`, `0x02`, `0x20`) for USB xHCI, GPU, NVMe, and SATA AHCI controllers. **Installs zero kernel drivers (`.sys`).** |

---

## 🔄 Safe Rollback (Windows IRQ Policies)

To revert Windows hardware interrupt steering back to stock Windows defaults, open PowerShell as **Administrator** and run:

    powershell -NoProfile -ExecutionPolicy Bypass -File "tools\set_irq_affinity.ps1" -Revert

Reboot Windows after running `-Revert` to restore default OS interrupt routing.

---

## 📬 Reporting a Vulnerability or Memory-Safety Bug

If you discover a memory-safety issue, race condition, or buffer overflow in `ax-sdk` (`include/ax/` or `src/`):

* Open an issue or private security advisory on **`galaxywarriordad/ax-sdk`** with reproduction steps, compiler flags, and your CPU/OS topology.
* Do **not** attach copyrighted game dumps, decryption keys, or firmware files to bug reports.
