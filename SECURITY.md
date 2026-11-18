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
| **Hardware MSI IRQ Steering** (`scripts/win32/set_irq_affinity.ps1`) | Windows PnP Registry (`Affinity Policy`) | **Administrator** (one-time registry write) | Writes `DevicePolicy = 4` and `AssignmentSetOverride` masks (`0x01`, `0x02`, `0x20`) for USB xHCI, GPU, NVMe, and SATA AHCI controllers. **Installs zero kernel drivers (`.sys`).** |

---

## 🔄 Safe Rollback (Windows IRQ Policies)

To revert Windows hardware interrupt steering back to stock Windows defaults, open PowerShell as **Administrator** and run:

```powershell
Get-ChildItem "HKLM:\SYSTEM\CurrentControlSet\Enum" -Recurse -ErrorAction SilentlyContinue |
  Where-Object { $_.PSChildName -eq "Affinity Policy" } |
  ForEach-Object {
      Remove-ItemProperty -Path $_.PSPath -Name "DevicePolicy","AssignmentSetOverride" -ErrorAction SilentlyContinue
  }
Write-Host "All custom IRQ affinity overrides cleared. Reboot Windows to restore default OS routing."
```

---

## 📬 Reporting a Vulnerability or Memory-Safety Bug

If you discover a memory-safety issue, race condition, or buffer overflow in `ax-sdk` (`include/ax/` or `src/`):

* Open an issue or private security advisory on **`galaxywarriordad/ax-sdk`** with reproduction steps, compiler flags, and your CPU/OS topology.
* Do **not** attach copyrighted game dumps, decryption keys, or firmware files to bug reports.
---
# 🛡️ Security Policy & Host System Privilege Notes

## ⚖️ Legal & Anti-Piracy Policy

**Eden Espresso Wave [AX]** is a clean-room, open-source software emulation and systems-tuning project licensed under **GPLv3 (or later)**.

* **Original Hardware Required:** In alignment with upstream **Eden** and **Citron** policies, you **must own an original Nintendo Switch console** and dump your own system firmware, cryptographic keys (`prod.keys` / `title.keys`), and legally purchased games from your own hardware.
* **Zero Proprietary Assets:** Any issue, pull request, or attachment containing copyrighted Nintendo keys, firmware binaries, ROMs/NSPs/XCIs, or proprietary third-party DLLs (`Lossless.dll`) will be rejected and removed immediately.

---

## 🔒 Host System Privileges & Kernel-Surface Audit

To achieve deterministic frame pacing and bit-perfect USB DAC audio on Windows 10/11, **Eden Espresso Wave [AX]** uses standard, documented Win32 user-mode APIs and Windows PnP Registry interrupt policies:

| Mechanism | Scope | Privilege Required | Security & Stability Impact |
| :--- | :--- | :--- | :--- |
| **`SeLockMemoryPrivilege`** (`VirtualAlloc` `MEM_LARGE_PAGES`) | User-mode `eden.exe` process | Standard user account once granted via `secpol.msc` (*Lock pages in memory*) | Backs guest fastmem reserves with 2 MB large pages to reduce Skylake DTLB misses. Falls back automatically to standard 4 KB pages if not granted. |
| **MMCSS `Pro Audio` & Thread Affinity** (`AvSetMmThreadCharacteristicsW`, `SetThreadAffinityMask`) | User-mode threads inside `eden.exe` | Standard user account (process-validated via `GetProcessAffinityMask`) | Pins audio, Vulkan, JIT, and quarantine threads to fixed CPU cores without elevating process token privileges. |
| **Hardware MSI IRQ Steering** (`Scripts/set_irq_affinity.ps1`) | Windows PnP Registry (`Affinity Policy`) | **Administrator** (one-time registry write) | Writes `DevicePolicy = 4` and `AssignmentSetOverride` masks (`0x01`, `0x02`, `0x20`) for USB xHCI, GPU, NVMe, and SATA AHCI controllers. **Installs zero kernel drivers (`.sys`).** |
| **Dynamic Third-Party DLL Loading** (`Lossless.dll`) | Optional user-selected path (`lsfg_dll_path`) | Standard user account | Loads only a user-supplied `Lossless.dll` path explicitly chosen in **Emulation → Configure → Graphics**. |

---

## 🔄 Safe Rollback & Uninstallation

If you ever wish to revert Windows hardware interrupt steering or large-page privileges back to stock Windows defaults:

### 1. Reset PCIe / USB / SATA IRQ Affinity Policies (Run PowerShell as Administrator)

```powershell
Get-ChildItem "HKLM:\SYSTEM\CurrentControlSet\Enum" -Recurse -ErrorAction SilentlyContinue |
  Where-Object { $_.PSChildName -eq "Affinity Policy" } |
  ForEach-Object {
      Remove-ItemProperty -Path $_.PSPath -Name "DevicePolicy","AssignmentSetOverride" -ErrorAction SilentlyContinue
  }
Write-Host "All custom IRQ affinity overrides cleared. Reboot Windows to restore default OS routing."
```

### 2. Revoke Large-Page Privilege (Optional)

Open `secpol.msc` → **Local Policies → User Rights Assignment → Lock pages in memory**, remove your user account, and reboot.

---

## 📬 Reporting a Vulnerability or Memory-Safety Bug

If you discover a memory-safety issue, buffer overflow, or privilege-handling bug in our custom subsystems (e.g., `vk_ubo_passthrough.cpp`, `flex_asio_manager.cpp`, `qsv_decoder.cpp`, or `thread.cpp`):

* Please open a private report or issue on **`galaxywarriordad/Eden-Espresso-Wave-AX`** with reproduction steps, your hardware topology, and relevant logs (`%APPDATA%\eden\log\eden_log.txt`).
* Do **not** attach copyrighted game dumps, decryption keys, or firmware files to bug reports.
