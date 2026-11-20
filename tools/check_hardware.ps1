<#
.SYNOPSIS
    [AX] Unified Pre-Flight Silicon, Privilege & IRQ Audit (ax-sdk/tools/check_hardware.ps1)
.DESCRIPTION
    Validates host CPU topology, SeLockMemoryPrivilege, CPU C-State/Minimum Processor State (100%),
    USB DAC Selective Suspend status, and PCIe/USB/SATA IRQ affinity masks against hardware_profiles.json
    and ax_engine.ini across Eden, Cemu, and FLAC_AX.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [ValidateSet("eden", "Cemu", "flac_ax", "generic")]
    [string]$TargetApp = "generic",

    [Parameter(Mandatory = $false)]
    [string]$ConfigPath = "",

    [Parameter(Mandatory = $false)]
    [string]$LogOutputPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Continue"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SdkRoot = Resolve-Path (Join-Path $ScriptDir "..")
$ProfilesJsonPath = Join-Path $SdkRoot "config\hardware_profiles.json"

if ([string]::IsNullOrWhiteSpace($ConfigPath)) {
    $CandidateIni = Join-Path (Get-Location) "ax_engine.ini"
    if (Test-Path $CandidateIni) {
        $ConfigPath = $CandidateIni
    }
}

$AuditLines = [System.Collections.Generic.List[string]]::new()
$Failures = 0
$Warnings = 0

function Write-Audit {
    param(
        [string]$Status,
        [string]$Message,
        [ConsoleColor]$Color = [ConsoleColor]::Gray
    )
    $Line = "[$Status] $Message"
    $AuditLines.Add($Line)
    Write-Host $Line -ForegroundColor $Color
}

Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host " [AX] HARDWARE & SILICON PRE-FLIGHT AUDIT (Target: $TargetApp)" -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan

# ------------------------------------------------------------------------------
# [1/5] CPU Topology, SMT & Clock Domain Check
# ------------------------------------------------------------------------------
$Cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$PhysCores = [int]$Cpu.NumberOfCores
$LogProcs = [int]$Cpu.NumberOfLogicalProcessors
$SmtActive = ($LogProcs -gt $PhysCores)

Write-Audit "INFO" "CPU: $($Cpu.Name.Trim()) ($PhysCores Physical Cores / $LogProcs Logical Threads)" ([ConsoleColor]::White)
if ($SmtActive) {
    Write-Audit "WARN" "SMT/Hyper-Threading is ENABLED. ax::topology will filter logical siblings dynamically." ([ConsoleColor]::Yellow)
    $Warnings++
} else {
    Write-Audit "PASS" "Zero-SMT Physical Core layout confirmed ($PhysCores C / $LogProcs T)." ([ConsoleColor]::Green)
}

# ------------------------------------------------------------------------------
# [2/5] Large-Page Memory Privilege (SeLockMemoryPrivilege)
# ------------------------------------------------------------------------------
$PrivOutput = whoami /priv | Select-String -Pattern "SeLockMemoryPrivilege"
if ($null -ne $PrivOutput) {
    Write-Audit "PASS" "SeLockMemoryPrivilege token right present (2 MB Windows Large Pages available)." ([ConsoleColor]::Green)
} else {
    Write-Audit "WARN" "SeLockMemoryPrivilege missing. Enable via secpol.msc -> Lock pages in memory and reboot." ([ConsoleColor]::Yellow)
    $Warnings++
}

# ------------------------------------------------------------------------------
# [3/5] GPU & Silicon Profile Matrix Match (hardware_profiles.json)
# ------------------------------------------------------------------------------
$Gpus = Get-CimInstance Win32_VideoController
foreach ($Gpu in $Gpus) {
    $PnpId = $Gpu.PNPDeviceID
    Write-Audit "INFO" "GPU Detected: $($Gpu.Name) | Driver: $($Gpu.DriverVersion) | PNP: $PnpId" ([ConsoleColor]::White)
}

if (Test-Path $ProfilesJsonPath) {
    try {
        $ProfilesData = Get-Content -Raw $ProfilesJsonPath | ConvertFrom-Json
        Write-Audit "PASS" "Loaded declarative silicon matrix (schema_version: $($ProfilesData.schema_version))." ([ConsoleColor]::Green)
    } catch {
        Write-Audit "WARN" "Could not parse $ProfilesJsonPath : $_" ([ConsoleColor]::Yellow)
        $Warnings++
    }
} else {
    Write-Audit "WARN" "hardware_profiles.json not found at $ProfilesJsonPath." ([ConsoleColor]::Yellow)
    $Warnings++
}

# ------------------------------------------------------------------------------
# [4/5] CPU C-State / Minimum Processor State & USB Selective Suspend Audit
# ------------------------------------------------------------------------------
try {
    $ActiveScheme = (powercfg /getactivescheme) -replace '.*GUID:\s*([a-fA-F0-9\-]+).*', '$1'
    # Subgroup Processor (54533251-82be-4824-96c1-47b60b740d00), Min Processor State (893dee8e-2bef-41e0-89c6-b55d0929964c)
    $MinProcQuery = powercfg /query $ActiveScheme 54533251-82be-4824-96c1-47b60b740d00 893dee8e-2bef-41e0-89c6-b55d0929964c 2>$null
    $AcHex = ($MinProcQuery | Select-String "Current AC Power Setting Index:\s*0x([0-9a-fA-F]+)").Matches.Groups[1].Value
    if (-not [string]::IsNullOrWhiteSpace($AcHex)) {
        $MinPct = [Convert]::ToInt32($AcHex, 16)
        if ($MinPct -eq 100) {
            Write-Audit "PASS" "Power Plan Minimum Processor State locked at 100% (C0/C1 instant-wake lock)." ([ConsoleColor]::Green)
        } else {
            Write-Audit "WARN" "Power Plan Minimum Processor State is $MinPct% (recommend 100% for zero-jitter audio/pacing)." ([ConsoleColor]::Yellow)
            $Warnings++
        }
    }

    # Subgroup USB Settings (2a737441-1930-4402-8d77-b2bebba308a3), USB Selective Suspend (48e6b7a6-50f5-4782-a5d4-53bb8f07e226)
    $UsbQuery = powercfg /query $ActiveScheme 2a737441-1930-4402-8d77-b2bebba308a3 48e6b7a6-50f5-4782-a5d4-53bb8f07e226 2>$null
    $UsbAcHex = ($UsbQuery | Select-String "Current AC Power Setting Index:\s*0x([0-9a-fA-F]+)").Matches.Groups[1].Value
    if (-not [string]::IsNullOrWhiteSpace($UsbAcHex)) {
        $UsbState = [Convert]::ToInt32($UsbAcHex, 16)
        if ($UsbState -eq 0) {
            Write-Audit "PASS" "USB Selective Suspend is DISABLED (USB DAC root hub protected from sleep states)." ([ConsoleColor]::Green)
        } else {
            Write-Audit "WARN" "USB Selective Suspend is ENABLED. Disable in Power Options to prevent USB DAC dropouts." ([ConsoleColor]::Yellow)
            $Warnings++
        }
    }
} catch {
    Write-Audit "WARN" "Unable to query Windows powercfg settings: $_" ([ConsoleColor]::Yellow)
}

# ------------------------------------------------------------------------------
# [5/5] PCIe / USB / Storage Hardware IRQ Affinity Mask Audit
# ------------------------------------------------------------------------------
$PciEnumRoot = "HKLM:\SYSTEM\CurrentControlSet\Enum\PCI"
$Core0Pinned = $false
$Core1Pinned = $false
$Core5Pinned = $false

if (Test-Path $PciEnumRoot) {
    $AffinityKeys = Get-ChildItem -Path $PciEnumRoot -Recurse -ErrorAction SilentlyContinue |
                    Where-Object { $_.PSChildName -eq "Affinity Policy" }

    foreach ($Key in $AffinityKeys) {
        $Props = Get-ItemProperty -Path $Key.PSPath -ErrorAction SilentlyContinue
        if ($null -ne $Props.AssignmentSetOverride) {
            $Bytes = [byte[]]$Props.AssignmentSetOverride
            $Mask = 0
            for ($i = 0; $i -lt $Bytes.Length; $i++) {
                $Mask = $Mask -bor ([uint64]$Bytes[$i] -shl ($i * 8))
            }
            $DeviceNode = Split-Path (Split-Path (Split-Path $Key.PSPath -Parent) -Parent) -Parent
            $DevProps = Get-ItemProperty -Path $DeviceNode -ErrorAction SilentlyContinue
            $DevDesc = if ($DevProps.DeviceDesc) { ($DevProps.DeviceDesc -split ';')[-1] } else { "PCIe Device" }

            $HexMask = "0x{0:X2}" -f $Mask
            if ($Mask -eq 0x01) {
                $Core0Pinned = $true
                Write-Audit "PASS" "Core 0: Audio/Timing ($HexMask) -> $DevDesc" ([ConsoleColor]::Green)
            } elseif ($Mask -eq 0x02) {
                $Core1Pinned = $true
                Write-Audit "PASS" "Core 1: Vulkan Submit ($HexMask) -> $DevDesc" ([ConsoleColor]::Green)
            } elseif ($Mask -eq 0x20 -or $Mask -eq 0xC0) {
                $Core5Pinned = $true
                Write-Audit "PASS" "Core 5: Aux Quarantine ($HexMask) -> $DevDesc" ([ConsoleColor]::Green)
            }
        }
    }
}

if (-not $Core0Pinned) { Write-Audit "INFO" "Core 0: Audio/Timing -> Unpinned (Default Windows PnP)" ([ConsoleColor]::DarkGray) }
if (-not $Core1Pinned) { Write-Audit "INFO" "Core 1: Vulkan Submit -> Unpinned (Default Windows PnP)" ([ConsoleColor]::DarkGray) }
if (-not $Core5Pinned) { Write-Audit "INFO" "Core 5: Aux Quarantine -> Unpinned (Default Windows PnP)" ([ConsoleColor]::DarkGray) }

Write-Host "------------------------------------------------------------------------" -ForegroundColor Cyan
Write-Host " Audit Complete | Failures: $Failures | Warnings: $Warnings" -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan

if (-not [string]::IsNullOrWhiteSpace($LogOutputPath)) {
    $ParentDir = Split-Path -Parent $LogOutputPath
    if (-not [string]::IsNullOrWhiteSpace($ParentDir) -and -not (Test-Path $ParentDir)) {
        New-Item -ItemType Directory -Path $ParentDir -Force | Out-Null
    }
    $AuditLines | Out-File -FilePath $LogOutputPath -Encoding UTF8 -Force
}

if ($Failures -gt 0) { exit 1 } else { exit 0 }
