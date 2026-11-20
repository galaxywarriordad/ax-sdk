# SPDX-FileCopyrightText: Copyright 2026 The Silicon Preservation Society
# SPDX-License-Identifier: MIT

param(
    [string]$BaselineJson = "baseline_telemetry.json",
    [string]$CurrentJson = "build_stage_telemetry_metrics.json",
    [string]$RepoRoot = (Get-Location).Path,
    [double]$MaxRammbVariancePct = 15.0,
    [int]$MaxAllowedHandles = 10000,
    [int]$MaxAllowedShaderStutters = 5
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\Get-AxProfile.ps1"

Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host " [AX] AUTOMATED PERFORMANCE REGRESSION CHECKER" -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan

if (!(Test-Path $CurrentJson)) {
    Write-Host "[WARN] No runtime telemetry snapshot found at $CurrentJson (cold compile stage). Skipping runtime regression gate." -ForegroundColor Yellow
    exit 0
}

$vramCeilingGiB = 5.50
$gpuLabel = "Default Maxwell"
try {
    $Hw = Get-AxHardwareProfile -RepoRoot $RepoRoot
    $vramCeilingGiB = [double]$Hw.Gpu.vram_ceiling_gib
    $gpuLabel = "$($Hw.GpuKey) [$($Hw.Gpu.architecture)]"
} catch {}

$current = Get-Content $CurrentJson -Encoding utf8 | ConvertFrom-Json
$hasFailed = $false

Write-Host "[INFO] Evaluating metrics for Process ID: $($current.ProcessId) ($gpuLabel | VRAM Ceiling: $vramCeilingGiB GiB)" -ForegroundColor Yellow
Write-Host "[INFO] Timestamp: $($current.Timestamp)" -ForegroundColor Yellow

# 1. Real-Time Audio Subsystem Validation Gating
if ($current.AudioUnderruns -gt 0 -or $current.VoicesDropped -gt 0) {
    Write-Host "[FAIL] Audio regression! AudioUnderruns: $($current.AudioUnderruns), VoicesDropped: $($current.VoicesDropped)" -ForegroundColor Red
    $hasFailed = $true
} else {
    Write-Host "[PASS] Audio subsystem healthy (0 underruns / dropped voices)." -ForegroundColor Green
}

# 2. Log-Parsed Shader Compilation Stutter Hard Ceiling Check
if ($null -ne $current.ShaderStutters -and $current.ShaderStutters -gt $MaxAllowedShaderStutters) {
    Write-Host "[FAIL] Shader regression! Compile stutters ($($current.ShaderStutters)) exceeded limit ($MaxAllowedShaderStutters)." -ForegroundColor Red
    $hasFailed = $true
} else {
    Write-Host "[PASS] Shader compilation cadence stable ($($current.ShaderStutters) stutters logged)." -ForegroundColor Green
}

# 3. Direct Leak Prevention Handle Count Barrier
if ($null -ne $current.HandleCount -and $current.HandleCount -gt $MaxAllowedHandles) {
    Write-Host "[FAIL] Resource handle leak! Open handles ($($current.HandleCount)) exceeded protection ceiling ($MaxAllowedHandles)." -ForegroundColor Red
    $hasFailed = $true
} else {
    Write-Host "[PASS] Core OS handle allocations clear ($($current.HandleCount) handles active)." -ForegroundColor Green
}

# 4. GPU-Aware VRAM Ceiling Gate (from ax-sdk hardware_profiles.json)
if ($null -ne $current.VramUsed_GiB -and [double]$current.VramUsed_GiB -gt $vramCeilingGiB) {
    Write-Host "[FAIL] VRAM ceiling exceeded! Used ($($current.VramUsed_GiB) GiB) > $gpuLabel limit ($vramCeilingGiB GiB)." -ForegroundColor Red
    $hasFailed = $true
}

# 5. Comparative Staging Evaluation Loops (RAM, CPU, and GPU Load Verification)
if (Test-Path $BaselineJson) {
    $baseline = Get-Content $BaselineJson -Encoding utf8 | ConvertFrom-Json
    $allowedMaxRam = $baseline.WorkingSet_MB * (1 + ($MaxRammbVariancePct / 100))

    Write-Host "[INFO] Performance Profile Delta Audits (Baseline vs Current):" -ForegroundColor Cyan
    Write-Host "       - CPU Load    : Baseline $($baseline.CpuUsage_Pct)% | Current $($current.CpuUsage_Pct)%" -ForegroundColor Gray
    Write-Host "       - Disk Stream : Baseline $($baseline.DiskIO_MBps) MB/s | Current $($current.DiskIO_MBps) MB/s" -ForegroundColor Gray
    Write-Host "       - GPU Compute : Baseline $($baseline.GpuUsage_Pct)% | Current $($current.GpuUsage_Pct)%" -ForegroundColor Gray
    Write-Host "       - Memory Pool : Baseline $($baseline.WorkingSet_MB) MB | Current $($current.WorkingSet_MB) MB" -ForegroundColor Gray

    if ($current.WorkingSet_MB -gt $allowedMaxRam) {
        Write-Host "[FAIL] Memory regression! Working set exceeded allowed variance ($MaxRammbVariancePct%)." -ForegroundColor Red
        $hasFailed = $true
    } else {
        Write-Host "[PASS] Memory working set within acceptable scaling limits." -ForegroundColor Green
    }
} else {
    Write-Host "[WARN] Baseline metrics file missing at $BaselineJson. Skipping historical drift verification." -ForegroundColor Yellow
}

# 6. Core Mapping Topology Visualization
$coreMap = $current.PerCoreThreads
if ($null -ne $coreMap) {
    Write-Host "[INFO] Per-Core Thread Affinity Distribution Matrix:" -ForegroundColor Cyan
    foreach ($key in $coreMap.PSObject.Properties.Name) {
        Write-Host "       - $key : $($coreMap.$key) threads" -ForegroundColor Gray
    }
}

# 7. Global Gate Result Evaluation Pipeline Termination
if ($hasFailed) {
    Write-Host "=======================================================================" -ForegroundColor Red
    Write-Host " [RESULT] BUILD FAILED PERFORMANCE REGRESSION THRESHOLDS" -ForegroundColor Red
    Write-Host "=======================================================================" -ForegroundColor Red
    exit 1
} else {
    Write-Host "=======================================================================" -ForegroundColor Green
    Write-Host " [RESULT] ALL PERFORMANCE REGRESSION CHECKS PASSED SUCCESSFULLY" -ForegroundColor Green
    Write-Host "=======================================================================" -ForegroundColor Green
    exit 0
}
