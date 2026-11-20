<#
.SYNOPSIS
    [AX] Cross-Application Async Telemetry Daemon (ax-sdk/tools/telemetry_daemon.ps1)
.DESCRIPTION
    Out-of-process low-overhead profiler for [AX] binaries (eden, Cemu, flac_ax).
    Reads SampleIntervalMs from ax_engine.ini when available, monitors Working Set,
    Private Memory, ProcessorAffinity, and optional log metrics, and outputs JSON telemetry.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$TargetProcess = "eden",

    [Parameter(Mandatory = $false)]
    [string]$ConfigPath = "ax_engine.ini",

    [Parameter(Mandatory = $false)]
    [string]$OutputJson = "build_stage_telemetry_metrics.json",

    [Parameter(Mandatory = $false)]
    [string]$LogFilePath = "",

    [Parameter(Mandatory = $false)]
    [int]$SampleIntervalMs = 0,

    [Parameter(Mandatory = $false)]
    [int]$MaxDurationSeconds = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "SilentlyContinue"

# ------------------------------------------------------------------------------
# 1. Resolve SampleIntervalMs from ax_engine.ini if not explicitly overridden
# ------------------------------------------------------------------------------
if ($SampleIntervalMs -le 0) {
    $SampleIntervalMs = 250
    if (Test-Path $ConfigPath) {
        $Match = Select-String -Path $ConfigPath -Pattern "^\s*SampleIntervalMs\s*=\s*(\d+)" | Select-Object -First 1
        if ($null -ne $Match) {
            $SampleIntervalMs = [int]$Match.Matches.Groups[1].Value
        }
    }
}

Write-Host "[AX TELEMETRY] Watching process '$TargetProcess' (Interval: ${SampleIntervalMs}ms -> $OutputJson)" -ForegroundColor Cyan

# Wait up to 60s for the target process to spawn
$WaitStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$Proc = $null
while ($null -eq $Proc -and $WaitStopwatch.Elapsed.TotalSeconds -lt 60) {
    $Proc = Get-Process -Name $TargetProcess -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $Proc) {
        Start-Sleep -Milliseconds 200
    }
}

if ($null -eq $Proc) {
    Write-Warning "[AX TELEMETRY] Timed out waiting for process '$TargetProcess' to start."
    exit 1
}

$SessionStopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$Samples = 0
$PeakWorkingSetMB = 0.0
$PeakPrivateBytesMB = 0.0
$SumWorkingSetMB = 0.0
$AudioUnderruns = 0
$VoicesDropped = 0
$LastAffinityHex = "0x0"

# ------------------------------------------------------------------------------
# 2. Non-Invasive Sampling Loop
# ------------------------------------------------------------------------------
while ($true) {
    $Proc.Refresh()
    if ($Proc.HasExited) {
        break
    }

    if ($MaxDurationSeconds -gt 0 -and $SessionStopwatch.Elapsed.TotalSeconds -ge $MaxDurationSeconds) {
        break
    }

    $WsMB = [Math]::Round($Proc.WorkingSet64 / 1MB, 2)
    $PrivMB = [Math]::Round($Proc.PrivateMemorySize64 / 1MB, 2)
    $LastAffinityHex = "0x{0:X}" -f ([uint64]$Proc.ProcessorAffinity)

    if ($WsMB -gt $PeakWorkingSetMB) { $PeakWorkingSetMB = $WsMB }
    if ($PrivMB -gt $PeakPrivateBytesMB) { $PeakPrivateBytesMB = $PrivMB }
    $SumWorkingSetMB += $WsMB
    $Samples++

    # Optional non-locking log tail scan for underrun / xrun counters if LogFilePath is provided
    if (-not [string]::IsNullOrWhiteSpace($LogFilePath) -and (Test-Path $LogFilePath)) {
        try {
            $Fs = [System.IO.File]::Open($LogFilePath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
            $Reader = New-Object System.IO.StreamReader($Fs)
            $TailContent = $Reader.ReadToEnd()
            $Reader.Close()
            $Fs.Close()

            $UnderrunMatches = [regex]::Matches($TailContent, "(?i)(audio_underrun|EPIPE|xrun)")
            $AudioUnderruns = $UnderrunMatches.Count
        } catch {
            # Ignore transient file locks
        }
    }

    Start-Sleep -Milliseconds $SampleIntervalMs
}

$SessionStopwatch.Stop()
$AvgWorkingSetMB = if ($Samples -gt 0) { [Math]::Round($SumWorkingSetMB / $Samples, 2) } else { 0.0 }

# ------------------------------------------------------------------------------
# 3. Emit Standardized [AX] Telemetry JSON Report
# ------------------------------------------------------------------------------
$Report = [ordered]@{
    schema_version         = "1.1"
    timestamp_utc          = (Get-Date).ToUniversalTime().ToString("o")
    target_process         = $TargetProcess
    sample_interval_ms     = $SampleIntervalMs
    samples_collected      = $Samples
    duration_seconds       = [Math]::Round($SessionStopwatch.Elapsed.TotalSeconds, 2)
    process_affinity_mask  = $LastAffinityHex
    avg_working_set_mb     = $AvgWorkingSetMB
    peak_working_set_mb    = $PeakWorkingSetMB
    peak_private_bytes_mb  = $PeakPrivateBytesMB
    audio_underruns        = $AudioUnderruns
    voices_dropped         = $VoicesDropped
}

$OutDir = Split-Path -Parent $OutputJson
if (-not [string]::IsNullOrWhiteSpace($OutDir) -and -not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
}

$Report | ConvertTo-Json -Depth 4 | Out-File -FilePath $OutputJson -Encoding UTF8 -Force
Write-Host "[AX TELEMETRY] Session complete ($Samples samples). Peak WS: ${PeakWorkingSetMB} MB | Underruns: $AudioUnderruns" -ForegroundColor Green
exit 0
