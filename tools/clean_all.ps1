<#
.SYNOPSIS
    [AX] Cross-Application Build & Cache Purge Utility (ax-sdk/tools/clean_all.ps1)
.DESCRIPTION
    Terminates lingering compiler/emulator processes, purges stale Ninja/CMake manifests
    and build trees (build, build_stage0..2, .cache), and supports -Wait so build scripts
    can block deterministically until disk deletion completes before invoking CMake.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$TargetRoot = (Get-Location).Path,

    [Parameter(Mandatory = $false)]
    [string[]]$ProcessNames = @("eden", "Cemu", "flac_ax", "ninja", "cmake", "clang-cl", "lld-link"),

    [Parameter(Mandatory = $false)]
    [string[]]$BuildDirs = @("build", "build_stage0", "build_stage1", "build_stage2", ".cache"),

    [Parameter(Mandatory = $false)]
    [switch]$Wait,

    [Parameter(Mandatory = $false)]
    [switch]$KeepProfiles
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "SilentlyContinue"

$ResolvedRoot = (Resolve-Path $TargetRoot).Path
Write-Host "========================================================================" -ForegroundColor Cyan
Write-Host " [AX CLEAN] Purging Build Artifacts in: $ResolvedRoot" -ForegroundColor Cyan
Write-Host "========================================================================" -ForegroundColor Cyan

# 1. Terminate active build/runtime handles that could lock files
foreach ($ProcName in $ProcessNames) {
    $Running = Get-Process -Name $ProcName -ErrorAction SilentlyContinue
    if ($null -ne $Running) {
        Write-Host "[STOP] Terminating active process: $ProcName" -ForegroundColor Yellow
        $Running | Stop-Process -Force -ErrorAction SilentlyContinue
    }
}

# 2. Remove root stray Ninja manifests, transient SPIR-V dumps, and telemetry logs
$RootArtifacts = @(
    ".ninja_deps",
    ".ninja_log",
    "frag.spv",
    "vert.spv",
    "build_stage_telemetry_metrics.json"
)

if (-not $KeepProfiles) {
    $RootArtifacts += @("default.profraw", "default.profdata", "eden.profdata")
}

foreach ($Item in $RootArtifacts) {
    $FullPath = Join-Path $ResolvedRoot $Item
    if (Test-Path $FullPath) {
        Remove-Item -Path $FullPath -Force -ErrorAction SilentlyContinue
        Write-Host "[DEL]  Removed root artifact: $Item" -ForegroundColor DarkGray
    }
}

Get-ChildItem -Path $ResolvedRoot -Filter "*.profraw" -File -ErrorAction SilentlyContinue | ForEach-Object {
    if (-not $KeepProfiles) {
        Remove-Item -Path $_.FullName -Force -ErrorAction SilentlyContinue
    }
}

# 3. Stage build directories into instant-rename trash folders for race-free cleanup
$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$TrashDirs = [System.Collections.Generic.List[string]]::new()

foreach ($DirName in $BuildDirs) {
    $TargetDir = Join-Path $ResolvedRoot $DirName
    if (Test-Path $TargetDir) {
        $TrashName = "_AXTRASH_${DirName}_${Timestamp}"
        $TrashPath = Join-Path $ResolvedRoot $TrashName
        try {
            Rename-Item -Path $TargetDir -NewName $TrashName -Force
            $TrashDirs.Add($TrashPath)
            Write-Host "[MOVE] Staged '$DirName' -> '$TrashName'" -ForegroundColor Green
        } catch {
            Write-Warning "[WARN] Could not rename '$DirName'; attempting direct removal."
            Remove-Item -Path $TargetDir -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

# Sweep any leftover _AXTRASH_* or EDENTRASH_* folders from prior interrupted runs
Get-ChildItem -Path $ResolvedRoot -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -like "_AXTRASH_*" -or $_.Name -like "EDENTRASH_*" } |
    ForEach-Object {
        if (-not $TrashDirs.Contains($_.FullName)) {
            $TrashDirs.Add($_.FullName)
        }
    }

# 4. Execute deletion synchronously (-Wait) or asynchronously in background
if ($TrashDirs.Count -gt 0) {
    $PathsArray = $TrashDirs.ToArray()
    if ($Wait) {
        Write-Host "[WAIT] Deleting $($PathsArray.Count) staged directory tree(s) synchronously..." -ForegroundColor Cyan
        foreach ($Trash in $PathsArray) {
            Remove-Item -Path $Trash -Recurse -Force -ErrorAction SilentlyContinue
        }
        Write-Host "[PASS] Synchronous purge complete." -ForegroundColor Green
    } else {
        Write-Host "[ASYNC] Spawning background deletion worker for $($PathsArray.Count) directory tree(s)..." -ForegroundColor Cyan
        $EncodedList = ($PathsArray | ForEach-Object { "'$_'" }) -join ","
        Start-Process -FilePath "powershell.exe" -WindowStyle Hidden -ArgumentList @(
            "-NoProfile",
            "-ExecutionPolicy", "Bypass",
            "-Command", "foreach (`$p in @($EncodedList)) { Remove-Item -Path `$p -Recurse -Force -ErrorAction SilentlyContinue }"
        ) | Out-Null
    }
} else {
    Write-Host "[PASS] Workspace already clean." -ForegroundColor Green
}

exit 0
