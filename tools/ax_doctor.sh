#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright 2026 The Silicon Preservation Society
# SPDX-License-Identifier: MIT
#
# ax_doctor.sh — Bare-Metal Linux Silicon, RT Kernel, HugePage & USB DAC Auditor
# Designed for Piccolo-AX and FLAC_AX appliances.

set -u

TARGET_APP="${1:-generic}"
FAILURES=0
WARNINGS=0

pass() { printf "  [PASS] %-48s -> %s\n" "$1" "$2"; }
warn() { printf "  [WARN] %-48s -> %s\n" "$1" "$2"; WARNINGS=$((WARNINGS + 1)); }
fail() { printf "  [FAIL] %-48s -> %s\n" "$1" "$2"; FAILURES=$((FAILURES + 1)); }
info() { printf "  [INFO] %-48s -> %s\n" "$1" "$2"; }

echo "======================================================================="
echo " [AX DOCTOR] Bare-Metal Linux Pre-Flight Silicon Audit (Target: $TARGET_APP)"
echo "======================================================================="

# ------------------------------------------------------------------------------
# [1/5] Real-Time Kernel & Boot Isolation Parameters
# ------------------------------------------------------------------------------
echo "[1/5] Auditing Kernel & Boot Isolation Parameters..."
UNAME_V="$(uname -v 2>/dev/null || true)"
CMDLINE="$(cat /proc/cmdline 2>/dev/null || true)"

if echo "$UNAME_V" | grep -q "PREEMPT_RT"; then
    pass "Kernel Preemption Model" "PREEMPT_RT active ($(uname -r))"
else
    warn "Kernel Preemption Model" "Standard kernel ($(uname -r)) — PREEMPT_RT recommended"
fi

if echo "$CMDLINE" | grep -q "isolcpus="; then
    ISOL_VAL="$(echo "$CMDLINE" | grep -oE 'isolcpus=[^ ]+')"
    pass "CPU Core Isolation (isolcpus)" "$ISOL_VAL"
else
    warn "CPU Core Isolation (isolcpus)" "Not set in /proc/cmdline (recommend isolcpus=managed_irq,domain,0-4)"
fi

if echo "$CMDLINE" | grep -q "nohz_full="; then
    NOHZ_VAL="$(echo "$CMDLINE" | grep -oE 'nohz_full=[^ ]+')"
    pass "Adaptive-Ticks Isolation (nohz_full)" "$NOHZ_VAL"
else
    warn "Adaptive-Ticks Isolation (nohz_full)" "Not set in /proc/cmdline (recommend nohz_full=0-4)"
fi

if echo "$CMDLINE" | grep -q "rcu_nocbs="; then
    RCU_VAL="$(echo "$CMDLINE" | grep -oE 'rcu_nocbs=[^ ]+')"
    pass "RCU Callback Offload (rcu_nocbs)" "$RCU_VAL"
else
    warn "RCU Callback Offload (rcu_nocbs)" "Not set in /proc/cmdline (recommend rcu_nocbs=0-4)"
fi

# ------------------------------------------------------------------------------
# [2/5] Physical CPU Topology & SMT Compliance
# ------------------------------------------------------------------------------
echo "[2/5] Auditing Physical CPU Topology & Frequency Governor..."
LOGICAL_CPUS="$(nproc 2>/dev/null || echo 1)"
SIBLINGS_0="$(cat /sys/devices/system/cpu/cpu0/topology/thread_siblings_list 2>/dev/null || echo 0)"
if [[ "$SIBLINGS_0" == *","* || "$SIBLINGS_0" == *"-"* ]]; then
    warn "SMT / Hyper-Threading" "Active (cpu0 siblings: $SIBLINGS_0) — disable SMT for 1:1 P-core lock"
else
    pass "Zero-SMT Physical Core Topology" "$LOGICAL_CPUS isolated physical cores (1:1 mapping)"
fi

GOV="$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo unknown)"
if [[ "$GOV" == "performance" ]]; then
    pass "CPU Frequency Governor" "performance (C-state frequency ramp locked)"
else
    warn "CPU Frequency Governor" "$GOV (recommend 'performance')"
fi

# ------------------------------------------------------------------------------
# [3/5] HugeTLB Fastmem Arenas (1G & 2M Pages)
# ------------------------------------------------------------------------------
echo "[3/5] Auditing HugeTLB Fastmem Pools..."
HP_1G="$(cat /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages 2>/dev/null || echo 0)"
HP_2M="$(cat /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages 2>/dev/null || echo 0)"
if [[ "$HP_1G" -ge 4 ]]; then
    pass "1 GiB HugePages Pool" "$HP_1G pages allocated ($((HP_1G)) GiB fastmem ready)"
elif [[ "$HP_2M" -ge 512 ]]; then
    pass "2 MiB HugePages Pool" "$HP_2M pages allocated ($((HP_2M * 2)) MiB fastmem ready)"
else
    warn "HugeTLB Fastmem Pool" "1G=$HP_1G, 2M=$HP_2M (emulators will fall back to 4K mmap)"
fi

# ------------------------------------------------------------------------------
# [4/5] Exclusive USB DAC & ALSA Real-Time Configuration
# ------------------------------------------------------------------------------
echo "[4/5] Auditing Exclusive USB DAC & ALSA Subsystem..."
NRPACKS="$(cat /sys/module/snd_usb_audio/parameters/nrpacks 2>/dev/null || echo unset)"
if [[ "$NRPACKS" == "1" ]]; then
    pass "snd_usb_audio nrpacks" "1 (minimum 125us USB isochronous packet batching)"
else
    warn "snd_usb_audio nrpacks" "$NRPACKS (recommend nrpacks=1 in modprobe / kernel cmdline)"
fi

if [[ -d /proc/asound/FC3 ]]; then
    pass "HiBy FC3 ESS Sabre DAC" "Detected at hw:CARD=FC3,DEV=0"
elif grep -qi "USB-Audio" /proc/asound/cards 2>/dev/null; then
    CARD_INFO="$(grep -i "USB-Audio" /proc/asound/cards | head -n 1 | xargs)"
    pass "USB Audio DAC Detected" "$CARD_INFO"
else
    warn "USB Audio DAC" "No USB-Audio card detected in /proc/asound/cards"
fi

# ------------------------------------------------------------------------------
# [5/5] Hardware IRQ Steering (/proc/interrupts) & Shared Memory Telemetry
# ------------------------------------------------------------------------------
echo "[5/5] Auditing IRQ Steering & Shared Telemetry..."
XHCI_IRQS="$(grep -E "xhci_hcd" /proc/interrupts 2>/dev/null | awk -F: '{print $1}' | xargs)"
if [[ -n "$XHCI_IRQS" ]]; then
    for irq in $XHCI_IRQS; do
        MASK="$(cat "/proc/irq/$irq/smp_affinity" 2>/dev/null || echo unknown)"
        if [[ "$MASK" == "01" || "$MASK" == "1" || "$MASK" == "00000001" ]]; then
            pass "USB xHCI IRQ $irq Affinity" "0x$MASK (Core 0 Audio/Timing locked)"
        else
            info "USB xHCI IRQ $irq Affinity" "0x$MASK (expected 0x01 on Core 0)"
        fi
    done
fi

if [[ -e /dev/shm/ax_telemetry ]]; then
    pass "Shared Memory Telemetry (/dev/shm/ax_telemetry)" "Active and mapped"
else
    info "Shared Memory Telemetry (/dev/shm/ax_telemetry)" "Not currently active (no running [AX] process)"
fi

echo "-----------------------------------------------------------------------"
echo " Audit Complete | Failures: $FAILURES | Warnings: $WARNINGS"
echo "======================================================================="

if [[ "$FAILURES" -gt 0 ]]; then
    exit 1
fi
exit 0
