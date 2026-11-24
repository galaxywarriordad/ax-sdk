[CmdletBinding()]
param()

$pass = 0; $warn = 0; $fail = 0
function Pass-Item([string]$m) { Write-Host "[PASS] $m" -ForegroundColor Green; $script:pass++ }
function Warn-Item([string]$m) { Write-Host "[WARN] $m" -ForegroundColor Yellow; $script:warn++ }
function Fail-Item([string]$m) { Write-Host "[FAIL] $m" -ForegroundColor Red; $script:fail++ }

$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$gpus = Get-CimInstance Win32_VideoController
$ramGb = [math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB, 2)

Write-Host "=== AX Hardware Self-Audit (Windows x86_64) ==="
Write-Host ("CPU: {0} | Cores: {1} | Logical: {2} | RAM: {3} GB" -f $cpu.Name.Trim(), $cpu.NumberOfCores, $cpu.NumberOfLogicalProcessors, $ramGb)

if ($cpu.NumberOfCores -eq $cpu.NumberOfLogicalProcessors) {
    Pass-Item "SMT/Hyper-Threading disabled (1:1 physical-to-logical core mapping)"
} else {
    Warn-Item ("Logical processors ({0}) exceed physical cores ({1}); verify 2:3:1 masks" -f $cpu.NumberOfLogicalProcessors, $cpu.NumberOfCores)
}

foreach ($g in $gpus) {
    $vramGiB = [math]::Round($g.AdapterRAM / 1GB, 2)
    if ($g.Name -match "980 Ti" -or $vramGiB -le 6.1) {
        Pass-Item ("GPU '{0}' (~{1} GiB) -> enforce 5.50 GiB VRAM guardrail" -f $g.Name, $vramGiB)
    } else {
        Pass-Item ("GPU '{0}' (~{1} GiB) detected" -f $g.Name, $vramGiB)
    }
}

$audioEndpoints = Get-CimInstance Win32_SoundDevice | Where-Object { $_.Status -eq "OK" }
if ($audioEndpoints.Name -match "HiBy|FC3|ES9281|ASIO|FlexASIO") {
    Pass-Item "USB DAC / ASIO audio endpoint detected"
} else {
    Warn-Item "No HiBy FC3 / ASIO endpoint explicitly named in Win32_SoundDevice"
}

Write-Host ("`nSummary: {0} PASS, {1} WARN, {2} FAIL" -f $pass, $warn, $fail)
if ($fail -gt 0) { exit 1 } else { exit 0 }
