<#
.SYNOPSIS
    [AX] Cross-Application Hardware IRQ Affinity Steering (ax-sdk/tools/set_irq_affinity.ps1)
.DESCRIPTION
    Steers PCIe MSI interrupts via Windows PnP Affinity Policy (DevicePolicy = 4):
      - Core 0 (0x01): USB xHCI Host Controller (Exclusive USB DAC audio isolation)
      - Core 1 (0x02): Discrete GPU (Vulkan / DirectX submission isolation)
      - Core 5 (0x20): NVMe & SATA AHCI Storage Controllers (Aux quarantine sink)
    Supports -Revert to restore default Windows PnP interrupt routing cleanly.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [switch]$Revert,

    [Parameter(Mandatory = $false)]
    [switch]$IncludeIntegratedGpu,

    [Parameter(Mandatory = $false)]
    [uint64]$AudioUsbMask = 0x01,

    [Parameter(Mandatory = $false)]
    [uint64]$DiscreteGpuMask = 0x02,

    [Parameter(Mandatory = $false)]
    [uint64]$StorageAuxMask = 0x20
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Verify Administrator privileges
$CurrentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $CurrentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "[FAIL] set_irq_affinity.ps1 must be run as Administrator."
    exit 1
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SdkRoot = Resolve-Path (Join-Path $ScriptDir "..")
$ProfilesJsonPath = Join-Path $SdkRoot "config\hardware_profiles.json"

if (Test-Path $ProfilesJsonPath) {
    try {
        $Profiles = Get-Content -Raw $ProfilesJsonPath | ConvertFrom-Json
        if ($null -ne $Profiles.default_masks) {
            if ($Profiles.default_masks.audio)        { $AudioUsbMask    = [Convert]::ToUInt64($Profiles.default_masks.audio, 16) }
            if ($Profiles.default_masks.vulkan)       { $DiscreteGpuMask = [Convert]::ToUInt64($Profiles.default_masks.vulkan, 16) }
            if ($Profiles.default_masks.housekeeping) { $StorageAuxMask  = [Convert]::ToUInt64($Profiles.default_masks.housekeeping, 16) }
        }
    } catch {
        Write-Warning "[WARN] Could not parse masks from $ProfilesJsonPath; using default 0x01 / 0x02 / 0x20."
    }
}

function Convert-MaskToBytes {
    param([uint64]$Mask)
    return [byte[]]@(
        [byte]($Mask -band 0xFF),
        [byte](($Mask -shr 8) -band 0xFF),
        [byte](($Mask -shr 16) -band 0xFF),
        [byte](($Mask -shr 24) -band 0xFF),
        [byte](($Mask -shr 32) -band 0xFF),
        [byte](($Mask -shr 40) -band 0xFF),
        [byte](($Mask -shr 48) -band 0xFF),
        [byte](($Mask -shr 56) -band 0xFF)
    )
}

function Set-DeviceIrqPolicy {
    param(
        [string]$InstanceId,
        [string]$FriendlyName,
        [uint64]$Mask,
        [bool]$Undo
    )

    $RegBase = "HKLM:\SYSTEM\CurrentControlSet\Enum\$InstanceId\Device Parameters\Interrupt Management"
    $AffinityPath = Join-Path $RegBase "Affinity Policy"
    $MsiPath = Join-Path $RegBase "MessageSignaledInterruptProperties"

    if (-not (Test-Path $RegBase)) {
        return
    }

    if ($Undo) {
        if (Test-Path $AffinityPath) {
            Remove-Item -Path $AffinityPath -Recurse -Force -ErrorAction SilentlyContinue
            Write-Host "[REVERT] Restored default PnP IRQ routing for: $FriendlyName" -ForegroundColor Yellow
        }
        return
    }

    if (-not (Test-Path $AffinityPath)) {
        New-Item -Path $AffinityPath -Force | Out-Null
    }

    # Enable MSI mode if supported by the device node
    if (-not (Test-Path $MsiPath)) {
        New-Item -Path $MsiPath -Force | Out-Null
    }
    Set-ItemProperty -Path $MsiPath -Name "MSISupported" -Type DWord -Value 1 -Force

    # DevicePolicy = 4 -> IrqPolicySpecifiedProcessors
    Set-ItemProperty -Path $AffinityPath -Name "DevicePolicy" -Type DWord -Value 4 -Force
    Set-ItemProperty -Path $AffinityPath -Name "AssignmentSetOverride" -Type Binary -Value (Convert-MaskToBytes -Mask $Mask) -Force

    $HexMask = "0x{0:X2}" -f $Mask
    Write-Host "[PINNED] $HexMask -> $FriendlyName ($InstanceId)" -ForegroundColor Green
}

Write-Host "========================================================================" -ForegroundColor Cyan
if ($Revert) {
    Write-Host " [AX] REVERTING HARDWARE IRQ AFFINITY POLICIES TO WINDOWS DEFAULTS" -ForegroundColor Yellow
} else {
    Write-Host " [AX] APPLYING HARDWARE IRQ AFFINITY POLICIES (Audio: 0x$("{0:X2}" -f $AudioUsbMask) | GPU: 0x$("{0:X2}" -f $DiscreteGpuMask) | Aux: 0x$("{0:X2}" -f $StorageAuxMask))" -ForegroundColor Cyan
}
Write-Host "========================================================================" -ForegroundColor Cyan

# 1. Discrete GPU (PCI Display Controllers — non-Intel by default)
$DisplayDevs = Get-PnpDevice -Class Display -Status OK -ErrorAction SilentlyContinue
foreach ($Dev in $DisplayDevs) {
    if ($Dev.InstanceId -notlike "PCI\*") { continue }
    $IsIntel = ($Dev.InstanceId -like "*VEN_8086*")
    if ($IsIntel -and -not $IncludeIntegratedGpu) {
        Set-DeviceIrqPolicy -InstanceId $Dev.InstanceId -FriendlyName $Dev.FriendlyName -Mask $StorageAuxMask -Undo $Revert.IsPresent
    } else {
        Set-DeviceIrqPolicy -InstanceId $Dev.InstanceId -FriendlyName $Dev.FriendlyName -Mask $DiscreteGpuMask -Undo $Revert.IsPresent
    }
}

# 2. USB xHCI Host Controllers (Isolates USB DAC interrupts on Core 0)
$UsbControllers = Get-PnpDevice -Class USB -Status OK -ErrorAction SilentlyContinue |
                  Where-Object { $_.InstanceId -like "PCI\*" -and ($_.FriendlyName -match "xHCI|Host Controller") }
foreach ($Dev in $UsbControllers) {
    Set-DeviceIrqPolicy -InstanceId $Dev.InstanceId -FriendlyName $Dev.FriendlyName -Mask $AudioUsbMask -Undo $Revert.IsPresent
}

# 3. NVMe & SATA AHCI Storage Controllers (Quarantined on Core 5)
$StorageControllers = Get-PnpDevice -Class SCSIAdapter,HDC -Status OK -ErrorAction SilentlyContinue |
                      Where-Object { $_.InstanceId -like "PCI\*" }
foreach ($Dev in $StorageControllers) {
    Set-DeviceIrqPolicy -InstanceId $Dev.InstanceId -FriendlyName $Dev.FriendlyName -Mask $StorageAuxMask -Undo $Revert.IsPresent
}

Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host " [NOTE] Reboot Windows for PnP Interrupt Affinity changes to take effect." -ForegroundColor White
Write-Host "========================================================================" -ForegroundColor Cyan
exit 0
