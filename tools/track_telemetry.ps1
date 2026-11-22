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

if ([string]::IsNullOrWhiteSpace($LogDir)) {
    if ($ProcessName -ieq "eden") {
        $LogDir = "$env:APPDATA\eden\log"
    } elseif ($ProcessName -ieq "Cemu") {
        $LogDir = "$env:APPDATA\Cemu"
    }
}

if ($SampleIntervalSeconds -le 0) {
    $SampleIntervalSeconds = 2
    if (Test-Path $ConfigPath) {
        $Match = Select-String -Path $ConfigPath -Pattern "^\s*SampleIntervalMs\s*=\s*(\d+)" | Select-Object -First 1
if ($SampleIntervalSeconds -le 0) {
    $SampleIntervalSeconds = 2
    if (Test-Path $ConfigPath) {
        $Match = Select-String -Path $ConfigPath -Pattern "^\s*SampleIntervalMs\s*=\s*(\d+)" | Select-Object -First 1
        if ($null -ne $Match -and $Match.Matches.Count -gt 0 -and $Match.Matches[0].Groups.Count -gt 1) {
            $parsedMs = 0
            if ([int]::TryParse($Match.Matches[0].Groups[1].Value, [ref]$parsedMs) -and $parsedMs -gt 0) {
                $SampleIntervalSeconds = [Math]::Max(1, [int][Math]::Round($parsedMs / 1000.0))
            }
        }
    }
}
    }
}

function Read-AxSharedTelemetry {
    try {
        $mmf = [System.IO.MemoryMappedFiles.MemoryMappedFile]::OpenExisting("Local\AX_Telemetry", [System.IO.MemoryMappedFiles.MemoryMappedFileRights]::Read)
        $acc = $mmf.CreateViewAccessor(0, 512, [System.IO.MemoryMappedFiles.MemoryMappedFileAccess]::Read)
        $magic = $acc.ReadUInt32(0)
        if ($magic -ne 0x4158544D) {
            $acc.Dispose()
            $mmf.Dispose()
            return $null
        }

        $res = [PSCustomObject]@{
            HostPid               = $acc.ReadUInt32(8)
            AudioUnderruns        = $acc.ReadUInt64(64)
            AudioBufferSwitches   = $acc.ReadUInt64(72)
            AudioHighWaterMark    = $acc.ReadUInt32(80)
            AudioProcessingTimeUs = $acc.ReadUInt32(84)
            VoicesDropped         = $acc.ReadUInt32(88)
            VramUsedBytes         = $acc.ReadUInt64(128)
            VramReaps             = $acc.ReadUInt64(136)
            Bar1Stalls            = $acc.ReadUInt64(144)
            IgpuOffloads          = $acc.ReadUInt64(152)
            QsvFramesDecoded      = $acc.ReadUInt64(160)
            FramesPresented       = $acc.ReadUInt64(192)
            LastFrametimeUs       = $acc.ReadUInt32(200)
            ShmWorkingSetMB       = $acc.ReadUInt32(204)
            ShmCommitSizeMB       = $acc.ReadUInt32(208)
            ShaderStutters        = $acc.ReadUInt32(212)
            AudioJitterMinUs      = $acc.ReadUInt32(256)
            AudioJitterMaxUs      = $acc.ReadUInt32(260)
            AudioJitterLastUs     = $acc.ReadUInt32(264)
            AudioRingLowWater     = $acc.ReadUInt32(268)
        }
        $acc.Dispose()
        $mmf.Dispose()
        return $res
    } catch {
        return $null
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
                        if (-not $prioMap.ContainsKey($key)) { $prioMap[$key] = 0 }
                        $prioMap[$key]++
                    } catch {}
                }
            } catch {}

            # Prefer zero-overhead live C++20 Shared Memory (Local\AX_Telemetry) when active
            $shm = Read-AxSharedTelemetry
            $shmActive = ($null -ne $shm -and $shm.HostPid -eq $proc.Id)

            $shaderStutters = 0
            $audioUnderruns = 0
            $voicesDropped = 0
            $vramUsedGiB = 0.0
            $vramReaps = 0
            $bar1Stalls = 0
            $igpuOffloads = 0
            $qsvFrames = 0
            $jitterMinUs = 0
            $jitterMaxUs = 0

            if ($shmActive) {
                $audioUnderruns = [int]$shm.AudioUnderruns
                $voicesDropped  = [int]$shm.VoicesDropped
                $shaderStutters = [int]$shm.ShaderStutters
                $vramUsedGiB    = [math]::Round($shm.VramUsedBytes / 1GB, 3)
                $vramReaps      = [int]$shm.VramReaps
                $bar1Stalls     = [int]$shm.Bar1Stalls
                $igpuOffloads   = [int]$shm.IgpuOffloads
                $qsvFrames      = [int]$shm.QsvFramesDecoded
                $jitterMinUs    = if ($shm.AudioJitterMinUs -eq 0xFFFFFFFF) { 0 } else { [int]$shm.AudioJitterMinUs }
                $jitterMaxUs    = [int]$shm.AudioJitterMaxUs
            }
            elseif (-not [string]::IsNullOrWhiteSpace($LogDir) -and (Test-Path $LogDir)) {
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
                Timestamp         = $timestamp
                TelemetrySource   = if ($shmActive) { "SharedMemory" } else { "LogFallback" }
                TargetProcess     = $ProcessName
                ProcessId         = $proc.Id
                WorkingSet_MB     = $workingSet
                ThreadCount       = $threadCount
                HandleCount       = $handles
                ThreadPriorities  = $prioMap
                AudioUnderruns    = $audioUnderruns
                VoicesDropped     = $voicesDropped
                AudioJitterMin_us = $jitterMinUs
                AudioJitterMax_us = $jitterMaxUs
                ShaderStutters    = $shaderStutters
                VramUsed_GiB      = $vramUsedGiB
                VramReaps         = $vramReaps
                Bar1Stalls        = $bar1Stalls
                IgpuOffloads      = $igpuOffloads
                QsvFramesDecoded  = $qsvFrames
                CpuUsage_Pct      = $cpuPct
                DiskIO_MBps       = 0
                GpuUsage_Pct      = 0
            }

            $metricJson = $metricObj | ConvertTo-Json -Compress
            [IO.File]::WriteAllText($OutputJsonPath, $metricJson, [Text.Encoding]::UTF8)

            Write-Host "[TELEMETRY] [$timestamp] $ProcessName (PID: $($proc.Id)) | CPU: ${cpuPct}% | RAM: ${workingSet}MB | VRAM: ${vramUsedGiB}GiB | JitterMax: ${jitterMaxUs}us | Underruns: $audioUnderruns" -ForegroundColor Green
        }
    }
    catch {
        Write-Host "[ERROR] Telemetry sampling error: $_" -ForegroundColor Red
    }

    Start-Sleep -Seconds $SampleIntervalSeconds
}
