# SPDX-FileCopyrightText: Copyright 2026 The Silicon Preservation Society
# SPDX-License-Identifier: MIT

param(
    [string]$ProcessName = "eden",
    [int]$SampleIntervalSeconds = 0,
    [string]$ConfigPath = "ax_engine.ini",
    [string]$OutputJsonPath = "build_stage_telemetry_metrics.json",
    [string]$LogDir = "",
    [string]$StopFilePath = "stop_telemetry"
)

$ErrorActionPreference = "Continue"

# Resolve default LogDir dynamically per application if not passed
if ([string]::IsNullOrWhiteSpace($LogDir)) {
    if ($ProcessName -ieq "eden") {
        $LogDir = "$env:APPDATA\eden\log"
    } elseif ($ProcessName -ieq "Cemu") {
        $LogDir = "$env:APPDATA\Cemu"
    }
}

# Resolve SampleIntervalSeconds from ax_engine.ini if not explicitly passed
if ($SampleIntervalSeconds -le 0) {
    $SampleIntervalSeconds = 2
    if (Test-Path $ConfigPath) {
        $Match = Select-String -Path $ConfigPath -Pattern "^\s*SampleIntervalMs\s*=\s*(\d+)" | Select-Object -First 1
        if ($null -ne $Match) {
            $ms = [int]$Match.Matches.Groups[1].Value
            $SampleIntervalSeconds = [Math]::Max(1, [int][Math]::Round($ms / 1000.0))
        }
    }
}

Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host " [AX] ASYNCHRONOUS PIPELINE TELEMETRY DAEMON ACTIVE ($ProcessName)" -ForegroundColor Cyan
Write-Host " Press Ctrl+C or create $StopFilePath to stop." -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan

$prevCpuSeconds = $null
$prevSampleTime = $null
$logicalCores = [Environment]::ProcessorCount

while ($true) {
    if (Test-Path $StopFilePath) {
        Write-Host "[STOP] Stop file detected. Exiting telemetry daemon." -ForegroundColor Yellow
        exit 0
    }

    try {
        $proc = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($null -eq $proc) {
            $prevCpuSeconds = $null
            $prevSampleTime = $null
            Write-Host "[WAITING] $ProcessName not currently running..." -ForegroundColor Yellow
        }
        else {
            $now = Get-Date
            $workingSet = [math]::Round($proc.WorkingSet64 / 1MB, 2)
            $handles = $proc.HandleCount

            $cpuPct = 0.0
            $curCpuSeconds = $proc.TotalProcessorTime.TotalSeconds
            if ($null -ne $prevCpuSeconds -and $null -ne $prevSampleTime) {
                $wallDelta = ($now - $prevSampleTime).TotalSeconds
                if ($wallDelta -gt 0) {
                    $cpuPct = [math]::Round((($curCpuSeconds - $prevCpuSeconds) / ($wallDelta * $logicalCores)) * 100.0, 2)
                }
            }
            $prevCpuSeconds = $curCpuSeconds
            $prevSampleTime = $now

            $prioMap = @{}
            $threadCount = 0
            try {
                foreach ($t in $proc.Threads) {
                    $threadCount++
                    try {
                        $prio = $t.PriorityLevel.ToString()
                        $key = "Prio_$prio"
                        if (-not $prioMap.ContainsKey($key)) {
                            $prioMap[$key] = 0
                        }
                        $prioMap[$key]++
                    } catch {}
                }
            } catch {}

            $shaderStutters = 0
            $audioUnderruns = 0
            $voicesDropped = 0

            if (-not [string]::IsNullOrWhiteSpace($LogDir) -and (Test-Path $LogDir)) {
                $candidateLogs = Get-ChildItem -Path $LogDir -Filter "*.log" -File -ErrorAction SilentlyContinue
                $txtLogs = Get-ChildItem -Path $LogDir -Filter "*_log.txt" -File -ErrorAction SilentlyContinue
                $allLogs = @($candidateLogs) + @($txtLogs) | Select-Object -Unique

                foreach ($logFile in $allLogs) {
                    $stutterMatches = Select-String -Path $logFile.FullName -Pattern "Shader compile|Pipeline creation took" -SimpleMatch -ErrorAction SilentlyContinue
                    if ($stutterMatches) { $shaderStutters += $stutterMatches.Count }

                    $underrunMatches = Select-String -Path $logFile.FullName -Pattern "underrun|EPIPE|buffer empty|xrun" -ErrorAction SilentlyContinue
                    if ($underrunMatches) { $audioUnderruns += $underrunMatches.Count }

                    $voiceMatches = Select-String -Path $logFile.FullName -Pattern "voice drop|dropped voice" -ErrorAction SilentlyContinue
                    if ($voiceMatches) { $voicesDropped += $voiceMatches.Count }
                }
            }

            $timestamp = $now.ToString("yyyy-MM-dd HH:mm:ss.fff")
            $metricObj = [PSCustomObject]@{
                Timestamp        = $timestamp
                TargetProcess    = $ProcessName
                ProcessId        = $proc.Id
                WorkingSet_MB    = $workingSet
                ThreadCount      = $threadCount
                HandleCount      = $handles
                ThreadPriorities = $prioMap
                AudioUnderruns   = $audioUnderruns
                VoicesDropped    = $voicesDropped
                ShaderStutters   = $shaderStutters
                CpuUsage_Pct     = $cpuPct
                DiskIO_MBps      = 0
                GpuUsage_Pct     = 0
            }

            $metricJson = $metricObj | ConvertTo-Json -Compress
            [IO.File]::WriteAllText($OutputJsonPath, $metricJson, [Text.Encoding]::UTF8)

            Write-Host "[TELEMETRY] [$timestamp] $ProcessName (PID: $($proc.Id)) | CPU: ${cpuPct}% | RAM: ${workingSet}MB | Handles: $handles | Threads: $threadCount | Underruns: $audioUnderruns" -ForegroundColor Green
        }
    }
    catch {
        Write-Host "[ERROR] Telemetry sampling error: $_" -ForegroundColor Red
    }

    Start-Sleep -Seconds $SampleIntervalSeconds
}
