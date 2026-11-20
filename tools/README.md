# 🛠️ `ax-sdk/tools/` — Aggregated Cross-Application Host Tooling

Shared, process-parameterized host auditing, IRQ steering, workspace cleanup, and telemetry scripts for all **`[AX]`** repositories (`Eden-Espresso-Wave-AX`, `Cemu-Latte-AX`, `Piccolo-AX`, and `FLAC_AX`).

All scripts resolve silicon thresholds from `../config/hardware_profiles.json` (`schema_version: 2.1`) and runtime parameters from `ax_engine.ini`.

---

## 📋 Available Scripts

* **`Get-AxProfile.ps1`** — Silicon profile resolver matching host CPU regex and GPU PCI Vendor/Device IDs against `config/hardware_profiles.json`.
* **`check_hardware.ps1`** — Pre-flight audit verifying `SeLockMemoryPrivilege`, CPU physical/SMT topology, PCIe/USB/SATA IRQ masks (`[Bus X:Y.Z]`), Power Plan Minimum Processor State (`100%`), and USB Selective Suspend (`Disabled`).
* **`set_irq_affinity.ps1`** — Administrative PCIe/USB/SATA MSI interrupt steering (`DevicePolicy = 4`) with `-Revert` rollback support.
* **`track_telemetry.ps1`** — Non-invasive, out-of-process sampler reading `SampleIntervalMs` from `ax_engine.ini` and writing standardized JSON telemetry.
* **`check_regression.ps1`** — Automated performance, handle-leak, audio underrun, and GPU VRAM ceiling (`vram_ceiling_gib`) gate.
* **`clean_all.ps1`** — Instant-rename (`EDENTRASH_*`) workspace purge supporting asynchronous background deletion or `-Wait` synchronous completion.

---

## 🚀 Usage Examples Across Repositories

### 1. From `Eden-Espresso-Wave-AX` (`PGO_config.bat` / `PGO_stage*.bat`)

    powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\clean_all.ps1" -ProjDir "%PROJ_DIR%" -Wait

    powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\check_hardware.ps1" -TargetApp eden -RepoRoot "%PROJ_DIR%"

    start "" /B powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\track_telemetry.ps1" -ProcessName eden -ConfigPath "ax_engine.ini"

### 2. From `Cemu-Latte-AX`

    powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\check_hardware.ps1" -TargetApp Cemu -RepoRoot "%PROJ_DIR%"

    start "" /B powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\track_telemetry.ps1" -ProcessName Cemu -ConfigPath "ax_engine.ini" -OutputJsonPath "cemu_telemetry.json"
