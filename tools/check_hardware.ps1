# SPDX-FileCopyrightText: Copyright 2026 The Silicon Preservation Society
# SPDX-License-Identifier: MIT

param(
    [string]$TargetApp = "generic",
    [string]$RepoRoot  = (Get-Location).Path
)

. "$PSScriptRoot\Get-AxProfile.ps1"

$audioMask   = [uint64]0x01
$vulkanMask  = [uint64]0x02
$storageMask = [uint64]0x20
$topoLabel   = "Elastic 2:3:1 Non-SMT"
$gpuArch     = "NVIDIA dGPU"

try {
    $Hw          = Get-AxHardwareProfile -RepoRoot $RepoRoot
    $audioMask   = [Convert]::ToUInt64($Hw.Cpu.masks.audio_irq, 16)
    $vulkanMask  = [Convert]::ToUInt64($Hw.Cpu.masks.vulkan_submit, 16)
    $storageMask = [Convert]::ToUInt64($Hw.Cpu.masks.storage_aux, 16)
    $topoLabel   = $Hw.Cpu.topology_mode
    $gpuArch     = $Hw.Gpu.architecture

    if (-not $Hw.SmtCompliant) {
        Write-Output ("  [WARN] Hyper-Threading/SMT ENABLED ({0}T > {1}T expected) - disable HT in BIOS for deterministic latency" -f $Hw.LogicalCores, $Hw.Cpu.expected_logical_cores)
    }
} catch {
    Write-Output "  [INFO] ax-sdk profile not resolved; using reference 2:3:1 masks (0x01/0x02/0x20)."
}

Write-Output "[1/5] Auditing Windows Large-Page Privilege (SeLockMemoryPrivilege)..."
$privCheck = whoami /priv | Select-String -Pattern "SeLockMemoryPrivilege"
if ($null -ne $privCheck) {
    Write-Output "  [PASS] SeLockMemoryPrivilege                           -> Present (2 MB Large Pages available)"
} else {
    Write-Output "  [WARN] SeLockMemoryPrivilege                           -> Missing (Enable in secpol.msc -> Lock pages in memory)"
}

Write-Output "[2/5] Auditing Active Silicon Profile ($TargetApp)..."
Write-Output ("  [INFO] Active Topology / GPU                           -> {0} | {1}" -f $topoLabel, $gpuArch)

Write-Output "[3/5] Auditing 1:1 PCIe/USB/SATA Hardware IRQ Steering ($topoLabel)..."

function Test-DeviceIrqAffinity {
    param(
        [string]$MatchName,
        [uint64]$ExpectedMask,
        [string]$PartitionLabel
    )

    $devices = Get-PnpDevice -Status OK -ErrorAction SilentlyContinue | Where-Object { $_.FriendlyName -like "*$MatchName*" }

    if (-not $devices) {
        Write-Output ("  [WARN] {0,-62} -> No active PnP device ({1})" -f $MatchName, $PartitionLabel)
        return
    }

    foreach ($dev in $devices) {
        $locProp = Get-PnpDeviceProperty -InstanceId $dev.InstanceId -KeyName "DEVPKEY_Device_LocationInfo" -ErrorAction SilentlyContinue
        $slotTag = if ($locProp -and $locProp.Data) {
            ($locProp.Data -replace '^PCI bus (\d+), device (\d+), function (\d+).*', ' [Bus $1:$2.$3]')
        } else { "" }
        $displayName = "$($dev.FriendlyName)$slotTag"

        $regPath = "HKLM:\SYSTEM\CurrentControlSet\Enum\$($dev.InstanceId)\Device Parameters\Interrupt Management\Affinity Policy"

        if (Test-Path $regPath) {
            $props = Get-ItemProperty -Path $regPath -ErrorAction SilentlyContinue
            if ($props.DevicePolicy -eq 4 -and $null -ne $props.AssignmentSetOverride) {
                $bytes = New-Object byte[] 8
                [Array]::Copy($props.AssignmentSetOverride, $bytes, [Math]::Min($props.AssignmentSetOverride.Length, 8))
                $actualMask = [BitConverter]::ToUInt64($bytes, 0)
                if ($actualMask -eq $ExpectedMask) {
                    Write-Output ("  [PASS] {0,-62} -> 0x{1:X2} ({2})" -f $displayName, $actualMask, $PartitionLabel)
                    continue
                }
            }
        }
        Write-Output ("  [WARN] {0,-62} -> Unpinned (Expected 0x{1:X2} for {2})" -f $displayName, $ExpectedMask, $PartitionLabel)
    }
}

Write-Output "  --- [SECTION 1/2] Real-Time Audio/Timing & Vulkan GPU Steering ---"
Test-DeviceIrqAffinity -MatchName "eXtensible Host Controller" -ExpectedMask $audioMask   -PartitionLabel "Audio/Timing"
Test-DeviceIrqAffinity -MatchName "NVIDIA GeForce"             -ExpectedMask $vulkanMask  -PartitionLabel "Vulkan Submit ($gpuArch)"

Write-Output "  --- [SECTION 2/2] Storage & DMA Interrupt Quarantine ---"
Test-DeviceIrqAffinity -MatchName "Standard NVM Express"       -ExpectedMask $storageMask -PartitionLabel "Aux Quarantine (NVMe)"
Test-DeviceIrqAffinity -MatchName "SATA AHCI"                  -ExpectedMask $storageMask -PartitionLabel "Aux Quarantine (SATA HDD)"

Write-Output "[4/5] Auditing CPU C-State Floor & USB DAC Selective Suspend..."
try {
    $activeScheme = (powercfg /getactivescheme) -replace '^.*GUID:\s*([0-9a-fA-F-]+).*$', '$1'
    $procMinOut   = powercfg /query $activeScheme SUB_PROCESSOR PROCTHROTTLEMIN | Select-String "Current AC Power Setting Index:"
    $usbSuspOut   = powercfg /query $activeScheme 2a737441-1930-4402-8d77-b2bebba308a3 48e6b7a6-50f5-4782-a5d4-53bb8f07e226 | Select-String "Current AC Power Setting Index:"

    if ($procMinOut -match '0x([0-9a-fA-F]+)') {
        $minPct = [Convert]::ToInt32($Matches[1], 16)
        if ($minPct -ge 100) {
            Write-Output ("  [PASS] CPU Minimum Processor State (AC)                -> {0}% (C0/C1 instant-wake locked)" -f $minPct)
        } else {
            Write-Output ("  [WARN] CPU Minimum Processor State (AC)                -> {0}% (Set to 100% to avoid C6/C7 100us wake spikes)" -f $minPct)
        }
    }

    if ($usbSuspOut -match '0x([0-9a-fA-F]+)') {
        $usbVal = [Convert]::ToInt32($Matches[1], 16)
        if ($usbVal -eq 0) {
            Write-Output "  [PASS] USB Selective Suspend (AC)                      -> Disabled (USB DAC 125us polling locked)"
        } else {
            Write-Output "  [WARN] USB Selective Suspend (AC)                      -> Enabled (Disable to prevent USB DAC micro-stutters)"
        }
    }
} catch {
    Write-Output "  [INFO] Power plan C-state/USB query skipped."
}

Write-Output "[5/5] Hardware Alignment Audit Complete."
Write-Output "[STATUS] Validated $gpuArch Hardware Alignment Map ($topoLabel)."
exit 0
