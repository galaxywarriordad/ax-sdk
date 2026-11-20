# SPDX-FileCopyrightText: Copyright 2026 The Silicon Preservation Society
# SPDX-License-Identifier: MIT

function Get-AxHardwareProfile {
    param(
        [string]$ProfileFile = "",
        [string]$CpuProfile  = "",
        [string]$GpuProfile  = "",
        [string]$RepoRoot    = (Get-Location).Path
    )

    if ([string]::IsNullOrWhiteSpace($ProfileFile)) {
        $SdkToolParent = Split-Path -Path $PSScriptRoot -Parent
        $Candidates = @(
            (Join-Path $SdkToolParent "config\hardware_profiles.json"),
            (Join-Path $RepoRoot "build_stage0\_deps\ax_sdk-src\config\hardware_profiles.json"),
            (Join-Path $RepoRoot "build_stage2\_deps\ax_sdk-src\config\hardware_profiles.json"),
            (Join-Path $RepoRoot "build_stage1\_deps\ax_sdk-src\config\hardware_profiles.json"),
            (Join-Path $RepoRoot "build\_deps\ax_sdk-src\config\hardware_profiles.json"),
            (Join-Path (Split-Path $RepoRoot -Parent) "ax-sdk\config\hardware_profiles.json")
        )
        if ($env:AX_SDK_DIR) {
            $Candidates = @((Join-Path $env:AX_SDK_DIR "config\hardware_profiles.json")) + $Candidates
        }
        foreach ($Cand in $Candidates) {
            if (Test-Path $Cand) {
                $ProfileFile = $Cand
                break
            }
        }
    }

    if ([string]::IsNullOrWhiteSpace($ProfileFile) -or -not (Test-Path $ProfileFile)) {
        throw "ax-sdk hardware_profiles.json not found. Run CMake FetchContent or set AX_SDK_DIR."
    }

    $Matrix       = Get-Content -Path $ProfileFile -Raw | ConvertFrom-Json
    $CpuInfo      = Get-CimInstance Win32_Processor | Select-Object -First 1
    $CpuName      = $CpuInfo.Name
    $LogicalCores = [int]$CpuInfo.NumberOfLogicalProcessors
    $PhysCores    = [int]$CpuInfo.NumberOfCores

    $GpuList      = Get-CimInstance Win32_VideoController
    $GpuNames     = ($GpuList.Name) -join " | "
    $GpuPnpIds    = ($GpuList.PNPDeviceID) -join " | "

    if ([string]::IsNullOrWhiteSpace($CpuProfile)) {
        foreach ($Prop in $Matrix.cpu_profiles.PSObject.Properties) {
            if ($CpuName -match $Prop.Value.match_regex) {
                $CpuProfile = $Prop.Name
                break
            }
        }
        if ([string]::IsNullOrWhiteSpace($CpuProfile)) {
            $CpuProfile = "coffee_lake_9600k"
        }
    }

    if ([string]::IsNullOrWhiteSpace($GpuProfile)) {
        foreach ($Prop in $Matrix.gpu_profiles.PSObject.Properties) {
            $Cand = $Prop.Value
            $PciMatched = $false
            foreach ($DevId in $Cand.pci_device_ids) {
                if ($GpuPnpIds -match "VEN_$($Cand.pci_vendor_id)&DEV_$DevId") {
                    $PciMatched = $true
                    break
                }
            }
            if ($PciMatched -or ($GpuNames -match $Cand.match_regex)) {
                $GpuProfile = $Prop.Name
                break
            }
        }
        if ([string]::IsNullOrWhiteSpace($GpuProfile)) {
            $GpuProfile = "maxwell_gtx980ti"
        }
    }

    return [PSCustomObject]@{
        ProfileFile   = $ProfileFile
        Matrix        = $Matrix
        CpuKey        = $CpuProfile
        GpuKey        = $GpuProfile
        Cpu           = $Matrix.cpu_profiles.$CpuProfile
        Gpu           = $Matrix.gpu_profiles.$GpuProfile
        CpuName       = $CpuName
        GpuNames      = $GpuNames
        PhysicalCores = $PhysCores
        LogicalCores  = $LogicalCores
        SmtCompliant  = ($LogicalCores -le $Matrix.cpu_profiles.$CpuProfile.expected_logical_cores)
    }
}
