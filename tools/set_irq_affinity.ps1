# SPDX-FileCopyrightText: Copyright 2026 The Silicon Preservation Society
# SPDX-License-Identifier: MIT
#Requires -RunAsAdministrator

param(
    [switch]$Revert,
    [string]$RepoRoot = (Get-Location).Path
)

. "$PSScriptRoot\Get-AxProfile.ps1"

$Hw          = Get-AxHardwareProfile -RepoRoot $RepoRoot
$audioMask   = [Convert]::ToUInt64($Hw.Cpu.masks.audio_irq, 16)
$vulkanMask  = [Convert]::ToUInt64($Hw.Cpu.masks.vulkan_submit, 16)
$storageMask = [Convert]::ToUInt64($Hw.Cpu.masks.storage_aux, 16)

if ($Revert) {
    Write-Host "[PROFILE] Reverting MSI IRQs to default Windows PnP routing..." -ForegroundColor Yellow
} else {
    Write-Host "[PROFILE] Steering MSI IRQs for '$($Hw.CpuKey)' [$($Hw.Cpu.topology_mode)] + '$($Hw.GpuKey)' [$($Hw.Gpu.architecture)]" -ForegroundColor Cyan
}

function Set-DeviceIrqAffinity {
    param(
        [string]$MatchName,
        [uint64]$Mask,
        [string]$Label,
        [bool]$Undo
    )
    $devices = Get-PnpDevice -Status OK | Where-Object { $_.FriendlyName -like "*$MatchName*" }
    if (-not $devices) {
        Write-Host "[WARN] No PnP device matched '$MatchName'" -ForegroundColor Yellow
        return
    }
    foreach ($dev in $devices) {
        $regPath = "HKLM:\SYSTEM\CurrentControlSet\Enum\$($dev.InstanceId)\Device Parameters\Interrupt Management\Affinity Policy"
        if ($Undo) {
            if (Test-Path $regPath) {
                Remove-Item -Path $regPath -Recurse -Force -ErrorAction SilentlyContinue
                Write-Host "[REVERT] Restored default PnP routing for '$($dev.FriendlyName)' ($Label)" -ForegroundColor Yellow
            }
            continue
        }
        if (-not (Test-Path $regPath)) {
            New-Item -Path $regPath -Force | Out-Null
        }
        New-ItemProperty -Path $regPath -Name "DevicePolicy" -Value 4 -PropertyType DWord -Force | Out-Null
        $bytes = [BitConverter]::GetBytes($Mask)
        New-ItemProperty -Path $regPath -Name "AssignmentSetOverride" -Value $bytes -PropertyType Binary -Force | Out-Null
        Write-Host "[OK] Steered '$($dev.FriendlyName)' ($Label) -> Mask 0x$($Mask.ToString('X2'))" -ForegroundColor Green
    }
}

# 1. Intel USB 3.0/3.10 eXtensible Host Controller (USB DAC)
Set-DeviceIrqAffinity -MatchName "eXtensible Host Controller" -Mask $audioMask   -Label "Audio/Timing" -Undo $Revert.IsPresent

# 2. Dedicated NVIDIA GPU
Set-DeviceIrqAffinity -MatchName "NVIDIA GeForce"             -Mask $vulkanMask  -Label "Vulkan Submit ($($Hw.Gpu.architecture))" -Undo $Revert.IsPresent

# 3. Standard NVM Express Controllers
Set-DeviceIrqAffinity -MatchName "Standard NVM Express"       -Mask $storageMask -Label "Aux Quarantine (NVMe)" -Undo $Revert.IsPresent

# 4. SATA AHCI Controller
Set-DeviceIrqAffinity -MatchName "SATA AHCI"                  -Mask $storageMask -Label "Aux Quarantine (SATA HDD)" -Undo $Revert.IsPresent

Write-Host "`nReboot Windows to apply PCIe/USB/SATA MSI interrupt affinity changes." -ForegroundColor Cyan
