# `ax-sdk/tools/` — Aggregated Cross-Application Host Tooling

Shared, process-parameterized host auditing, IRQ steering, workspace cleanup, and telemetry scripts for all `[AX]` repositories (`Eden-Espresso-Wave-AX`, `Cemu-Latte-AX`, `Piccolo-AX`, `Piccolo-2go`, `piccolo-ax-desktop`, `FLAC_AX`, `Amiberry-AX`, and `Generation-X-AX`).

All scripts resolve silicon thresholds from `../config/hardware_profiles.json` (`schema_version: 2.1`) and runtime parameters from `ax_engine.ini`.

---

## Deterministic Memory & Diagnostic Principles

* **Explicit memory contracts over AI/ML:** RAM, HugeTLB (`1G` / `2M`), LZ4 `zram`, `toram` `/dev/shm` `EROFS` (`< 512 MB`) staging, and `VkVramShield` byte ceilings are governed by deterministic rules and fixed budgets rather than dynamic AI/ML guessing or inference daemons—avoiding background wakeups, core jitter, and RAM/VRAM consumption on isolated real-time systems.  
* **Read-only diagnostic & pre-flight role for `ax_doctor`:** `ax_doctor` acts strictly as an on-demand, read-only diagnostic and pre-flight auditor that checks system invariants (kernel isolation cmdline flags, `MemAvailable` and HugeTLB/zram/toram headroom, IRQ affinity masks, and USB DAC / lock readiness) and reports exact values or pass/fail status without polling in the background or silently mutating state.  
* **Separation from release snapshots & offline dev tooling:** Keep archive/tag-pinning checks distinct from live runtime pre-flight checks, and restrict any AI/log-analysis helpers strictly to offline CPU/RAM development/CI hosts outside the target appliance image.

---

## Available Scripts

* **`Get-AxProfile.ps1`** — Silicon profile resolver matching host CPU regex and GPU PCI Vendor/Device IDs against `config/hardware_profiles.json`.  
* **`check_hardware.ps1`** — Pre-flight audit verifying `SeLockMemoryPrivilege`, CPU physical/SMT topology, PCIe/USB/SATA IRQ masks (`[Bus X:Y.Z]`), Power Plan Minimum Processor State (`100%`), and USB Selective Suspend (`Disabled`).  
* **`set_irq_affinity.ps1`** — Administrative PCIe/USB/SATA MSI interrupt steering (`DevicePolicy = 4`) with `-Revert` rollback support.  
* **`track_telemetry.ps1`** — Non-invasive, out-of-process sampler reading `SampleIntervalMs` from `ax_engine.ini` and writing standardized JSON telemetry.  
* **`check_regression.ps1`** — Automated performance, handle-leak, audio underrun, and GPU VRAM ceiling (`vram_ceiling_gib`) gate.  
* **`clean_all.ps1`** — Instant-rename (`EDENTRASH_*`) workspace purge supporting asynchronous background deletion or `-Wait` synchronous completion.  
* **`ax_doctor.sh`** — Read-only, on-demand bare-metal Linux (`6.12-rt` / `Piccolo-AX`) pre-flight auditor for `nohz_full` / `isolcpus` / `rcu_nocbs` cmdline flags, RAM / `toram` / HugeTLB / `zram` state, IRQ affinity, and DAC / lock readiness (distinct from release snapshot/tag verification).

---

## Usage Examples Across Repositories

### 1\. From `Eden-Espresso-Wave-AX` (`PGO_config.bat` / `PGO_stage*.bat`)

```
powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\clean_all.ps1" -ProjDir "%PROJ_DIR%" -Wait
powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\check_hardware.ps1" -TargetApp eden -RepoRoot "%PROJ_DIR%"
start "" /B powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\track_telemetry.ps1" -ProcessName eden -ConfigPath "ax_engine.ini"
```

### 2\. From `Cemu-Latte-AX`

```
powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\check_hardware.ps1" -TargetApp Cemu -RepoRoot "%PROJ_DIR%"
start "" /B powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\track_telemetry.ps1" -ProcessName Cemu -ConfigPath "ax_engine.ini" -OutputJsonPath "cemu_telemetry.json"
```

