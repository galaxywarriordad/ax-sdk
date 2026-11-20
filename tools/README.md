# 🛠️ `ax-sdk/tools/` — Aggregated Cross-Application Host Tooling

Shared, process-parameterized host auditing, IRQ steering, workspace cleanup, and telemetry scripts for all **`[AX]`** repositories (`Eden-Espresso-Wave-AX`, `Cemu-Latte-AX`, `Piccolo-AX`, and `FLAC_AX`).

All scripts resolve silicon thresholds from `../config/hardware_profiles.json` (`schema_version: 2.1`) and runtime parameters from `ax_engine.ini`.

---

## 📋 Available Scripts

* **`check_hardware.ps1`** — Pre-flight audit verifying CPU physical/SMT topology, `SeLockMemoryPrivilege` (2 MB Large Pages), Power Plan Minimum Processor State (`100%` C-state lock), USB DAC Selective Suspend (`Disabled`), and PCIe/USB/SATA IRQ affinity masks (`0x01`, `0x02`, `0x20`).
* **`telemetry_daemon.ps1`** — Non-invasive, out-of-process sampler that reads `SampleIntervalMs` from `ax_engine.ini` and tracks Working Set, Private Bytes, Processor Affinity, and audio underruns/xruns into a standardized JSON report.

---

## 🚀 Usage Examples Across Repositories

### 1. From `Eden-Espresso-Wave-AX` (`PGO_config.bat` / `PGO_stage*.bat`)

Run the pre-flight hardware audit and launch the background telemetry sampler before starting `eden.exe`:
    
    powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\check_hardware.ps1" -TargetApp eden -LogOutputPath "%HW_CHECK_LOG%"

    start "" /B powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\telemetry_daemon.ps1" -TargetProcess eden -ConfigPath "ax_engine.ini" -OutputJson "build_stage_telemetry_metrics.json"

### 2. From `Cemu-Latte-AX`

Run the pre-flight audit and attach the telemetry daemon to `Cemu.exe`:
    
    powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\check_hardware.ps1" -TargetApp Cemu

    start "" /B powershell -NoProfile -ExecutionPolicy Bypass -File "%AX_SDK_DIR%\tools\telemetry_daemon.ps1" -TargetProcess Cemu -ConfigPath "ax_engine.ini" -OutputJson "cemu_telemetry.json"

### 3. From `FLAC_AX` (or Linux `Piccolo-AX` Shell Wrapper)

When running on Windows or cross-testing via PowerShell Core (`pwsh`):
    
    pwsh -NoProfile -File "$env:AX_SDK_DIR/tools/telemetry_daemon.ps1" -TargetProcess flac_ax -ConfigPath "ax_engine.ini" -OutputJson "flac_telemetry.json"
