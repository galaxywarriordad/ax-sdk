#!/usr/bin/env bash
set -u

PASS=0
WARN=0
FAIL=0

ok()   { printf "[PASS] %s\n" "$1"; PASS=$((PASS + 1)); }
warn() { printf "[WARN] %s\n" "$1"; WARN=$((WARN + 1)); }
fail() { printf "[FAIL] %s\n" "$1"; FAIL=$((FAIL + 1)); }

ARCH="$(uname -m)"
CMDLINE="$(cat /proc/cmdline 2>/dev/null || true)"
printf "=== AX Doctor Self-Audit (%s / %s) ===\n" "$ARCH" "$(uname -r)"

# 1. Kernel isolation flags (2:3:1 x86_64 vs 1:2:1 RK3326 aarch64)
case "$ARCH" in
  aarch64)
    if grep -qE 'rk3326' /proc/device-tree/compatible 2>/dev/null; then
      for flag in "isolcpus=0,1,2" "nohz_full=0,1,2" "rcu_nocbs=0,1,2" "threadirqs"; do
        if [[ "$CMDLINE" == *"$flag"* ]]; then
          ok "RK3326 boot arg present: $flag"
        else
          fail "RK3326 missing boot arg: $flag"
        fi
      done
    else
      warn "aarch64 host is not RK3326; skipping strict 0,1,2 isolation check"
    fi
    ;;
  x86_64)
    for flag in "isolcpus=" "nohz_full=" "rcu_nocbs="; do
      if [[ "$CMDLINE" == *"$flag"* ]]; then
        ok "x86_64 isolation arg active: ${flag%%=*}"
      else
        warn "x86_64 missing kernel isolation arg: ${flag%%=*}"
      fi
    done
    ;;
  *)
    warn "Unrecognized architecture: $ARCH"
    ;;
esac

# 2. IRQ affinity check (USB / DWC2 / I2S / MMC)
if [[ -r /proc/irq/default_smp_affinity ]]; then
  ok "Default IRQ affinity mask: $(cat /proc/irq/default_smp_affinity)"
fi
for irq_dir in /proc/irq/[0-9]*; do
  [[ -d "$irq_dir" ]] || continue
  irq_num="${irq_dir##*/}"
  desc="$(find "$irq_dir" -maxdepth 1 -type d ! -name . 2>/dev/null | head -n1 | xargs -r basename)"
  aff="$(cat "$irq_dir/smp_affinity" 2>/dev/null || true)"
  case "$desc" in
    *xhci*|*dwc2*|*i2s*)
      [[ "$aff" == *"1" ]] && ok "IRQ $irq_num ($desc) pinned toward Core 0 (mask $aff)" || warn "IRQ $irq_num ($desc) mask is $aff (expected Core 0)"
      ;;
    *mmc*|*dwmmc*)
      [[ "$aff" == *"1" ]] && warn "Storage IRQ $irq_num ($desc) touches Core 0 (mask $aff)" || ok "Storage IRQ $irq_num ($desc) off Core 0 (mask $aff)"
      ;;
  esac
done

# 3. SCHED_FIFO priority 80 privilege check
rtprio_limit="$(ulimit -r 2>/dev/null || echo 0)"
if [[ "$rtprio_limit" -ge 80 || "$(id -u)" -eq 0 ]]; then
  ok "SCHED_FIFO priority 80 permitted (ulimit -r: $rtprio_limit)"
else
  fail "SCHED_FIFO priority 80 blocked (ulimit -r: $rtprio_limit; check /etc/security/limits.d/)"
fi

# 4. HugeTLB & ZRAM check
huge_total="$(awk '/HugePages_Total:/ {print $2}' /proc/meminfo 2>/dev/null || echo 0)"
huge_sz="$(awk '/Hugepagesize:/ {print $2}' /proc/meminfo 2>/dev/null || echo 0)"
if [[ "$huge_total" -gt 0 ]]; then
  ok "HugeTLB allocated: ${huge_total} pages (${huge_sz} kB each)"
else
  warn "HugePages_Total is 0 (check vm.nr_hugepages or ax_hugepage_shim.so)"
fi

if [[ -e /dev/zram0 ]]; then
  zram_mb="$(( $(cat /sys/block/zram0/disksize 2>/dev/null || echo 0) / 1048576 ))"
  zram_comp="$(cat /sys/block/zram0/comp_algorithm 2>/dev/null || true)"
  ok "ZRAM active: ${zram_mb} MB ($zram_comp), swappiness=$(cat /proc/sys/vm/swappiness 2>/dev/null || echo n/a)"
else
  warn "/dev/zram0 not detected"
fi

# 5. Exclusive DAC lockfile check (/run/lock/ax-dac.lock)
lock_file="/run/lock/ax-dac.lock"
if [[ -e "$lock_file" ]]; then
  holder="$(fuser "$lock_file" 2>/dev/null | tr -s ' ' || true)"
  if [[ -n "$holder" ]]; then
    warn "DAC lock held by PID(s):$holder ($lock_file)"
  else
    warn "Lockfile exists without active holder (stale lock): $lock_file"
  fi
else
  ok "DAC lockfile clear (/run/lock/ax-dac.lock)"
fi

# 6. HiBy FC3 & RK817 mixer / gain-staging check
if command -v aplay >/dev/null 2>&1; then
  cards="$(aplay -l 2>/dev/null || true)"
  if printf "%s" "$cards" | grep -qiE "FC3|HiBy|ES9281"; then
    ok "HiBy FC3 / ESS Sabre DAC detected on ALSA"
    if command -v amixer >/dev/null 2>&1; then
      card_id="$(aplay -l 2>/dev/null | awk -F'[][]' '/FC3|HiBy|ES9281/ {print $2; exit}')"
      [[ -n "$card_id" ]] && amixer -c "$card_id" sget PCM 2>/dev/null | awk '/Mono:|Front Left:/ {print "  [INFO] Mixer:", $0}' || true
    fi
  else
    warn "No HiBy FC3 / ES9281 USB DAC detected via aplay -l"
  fi
fi

if printf "%s" "$cards" | grep -qiE "rk817|rockchip"; then
  ok "RK817 / Rockchip audio codec visible in ALSA"
fi

printf "\nSummary: %d PASS, %d WARN, %d FAIL\n" "$PASS" "$WARN" "$FAIL"
[[ "$FAIL" -eq 0 ]] && exit 0 || exit 1
