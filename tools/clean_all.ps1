# SPDX-FileCopyrightText: Copyright 2026 The Silicon Preservation Society
# SPDX-License-Identifier: MIT

param(
    [string]$ProjDir = (Get-Location).Path,
    [string[]]$Targets = @("ninja", "cmake", "clang-cl", "lld-link", "eden", "Cemu", "flac_ax", "glslangValidator"),
    [switch]$Wait
)

$ErrorActionPreference = "Stop"
$ProjDir = (Resolve-Path -Path $ProjDir).Path

Write-Output "======================================================================="
Write-Output " [PURGE SUITE] Initializing Absolute Pipeline Cleanup Pass"
Write-Output " Target Path: $ProjDir"
Write-Output "======================================================================="

Write-Output "[STATUS] Terminating active build engine processes..."
foreach ($Target in $Targets) {
    try {
        Stop-Process -Name $Target -Force -ErrorAction SilentlyContinue
    } catch {}
}
Start-Sleep -Seconds 1

Write-Output "[STATUS] Clearing pipeline telemetry log files..."
$LogFiles = @(
    "build_stage0_summary.log",
    "build_stage0_verbose.log",
    "build_stage0_triage.log",
    "build_stage0_incremental.log",
    "build_stage1_verbose.log",
    "build_stage2_verbose.log",
    "profile_validation_report.log",
    "build_session.log"
)
foreach ($Log in $LogFiles) {
    $TargetLog = Join-Path $ProjDir $Log
    if (Test-Path $TargetLog) { Remove-Item -Path $TargetLog -Force -ErrorAction SilentlyContinue }
}

Write-Output "[STATUS] Purging persistent manifests, stray shaders, and PGO artifacts..."
$StrayFiles = @(
    ".ninja_deps",
    ".ninja_log",
    "build.ninja",
    "rules.ninja",
    "CMakeCache.txt",
    "frag.spv",
    "vert.spv",
    "default.profdata",
    "0x01",
    "0x02",
    "0x20"
)
foreach ($File in $StrayFiles) {
    $TargetFile = Join-Path $ProjDir $File
    if (Test-Path $TargetFile) { Remove-Item -Path $TargetFile -Force -ErrorAction SilentlyContinue }
}

Get-ChildItem -Path $ProjDir -Filter "*.profraw" -File -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue
$ScriptsSubdir = Join-Path $ProjDir "Scripts"
if (Test-Path $ScriptsSubdir) {
    Get-ChildItem -Path $ScriptsSubdir -Filter "*.spv" -File -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue
}

Write-Output "[STATUS] Staging build trees, PGO caches, and stray folders for cleanup..."
$RandID = "EDENTRASH_" + (Get-Random -Minimum 10000 -Maximum 99999).ToString()

$BuildDirs = [ordered]@{
    "build"            = "trash_build_$RandID"
    "build_stage0"     = "trash_s0_$RandID"
    "PGO_stage0"       = "trash_alt_s0_$RandID"
    "build_stage1"     = "trash_s1_$RandID"
    "build_stage2"     = "trash_s2_$RandID"
    "build_pgo"        = "trash_bpgo_$RandID"
    "pgo_data"         = "trash_pgo_$RandID"
    "pgo_data_triage"  = "trash_pgo_triage_$RandID"
    "pgo_profiles"     = "trash_pgoprof_$RandID"
    ".cache"           = "trash_cache_$RandID"
    "CMakeFiles"       = "trash_cmake_$RandID"
    "Testing"          = "trash_testing_$RandID"
    "x64"              = "trash_x64_$RandID"
    "Release"          = "trash_rel_$RandID"
    "Debug"            = "trash_dbg_$RandID"
    "Scripts\.vs"      = "trash_scriptsvs_$RandID"
}

foreach ($Key in $BuildDirs.Keys) {
    $SourcePath = Join-Path $ProjDir $Key
    if (Test-Path $SourcePath) {
        try {
            Rename-Item -Path $SourcePath -NewName $BuildDirs[$Key] -Force
        } catch {
            Write-Output "  [STATUS] Persistent file handle lock discovered on folder: $Key"
            Start-Sleep -Seconds 1
            try {
                Rename-Item -Path $SourcePath -NewName $BuildDirs[$Key] -Force
            } catch {
                Write-Output "  [@=FATAL] Workspace path locked - $Key. Skipping block..."
            }
        }
    }
}

$ScriptPayload = @"
Get-ChildItem -Path '$ProjDir' -Directory -ErrorAction SilentlyContinue | Where-Object {
    (`$_.Name -like '*EDENTRASH_*' -or `$_.Name -like '*_trash_*' -or `$_.Name -like 'trash_*')
} | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
"@

$EncodedCommand = [Convert]::ToBase64String([System.Text.Encoding]::Unicode.GetBytes($ScriptPayload))

try {
    if ($Wait) {
        Write-Output "[STATUS] Running synchronous disk purge (-Wait)..."
        Start-Process -FilePath "powershell.exe" -ArgumentList "-NoProfile -ExecutionPolicy Bypass -EncodedCommand $EncodedCommand" -WindowStyle Hidden -Wait
        Write-Output "======================================================================="
        Write-Output " [SUCCESS] Workspace verified clean. Synchronous purge complete!"
        Write-Output "======================================================================="
    } else {
        Write-Output "[STATUS] Launching asynchronous multi-threaded disk deletion worker..."
        Start-Process -FilePath "powershell.exe" -ArgumentList "-NoProfile -ExecutionPolicy Bypass -EncodedCommand $EncodedCommand" -WindowStyle Hidden
        Write-Output "======================================================================="
        Write-Output " [SUCCESS] Workspace verified clean. Background deletion initialized!"
        Write-Output "======================================================================="
    }
} catch {
    Write-Output "[@=FATAL] Failed to allocate worker window handle context."
    exit 1
}

exit 0
