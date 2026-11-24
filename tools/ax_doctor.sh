#!/usr/bin/env bash
set -u

PASS=0
WARN=0
FAIL=0

pass() { printf "[PASS] %s: %s\n" "$1" "$2"; PASS=$((PASS + 1)); }
warn() { printf "[WARN] %s: %s\n" "$1" "$2"; WARN=$((WARN + 1)); }
fail() { printf "[FAIL] %s: %s\n" "$1" "$2"; FAIL=$((FAIL + 1)); }
info() { printf "[INFO] %s: %s\n" "$1" "$2"; }

ARCH="$(uname -m)"
KERNEL_REL="$(uname -r)"
CMDLINE="$(cat /proc/cmdline 2>/dev/null || true)"
VERSION_STR="$(cat /proc/version 2>/dev/null || true)"

printf "=== AX Doctor Self-Audit (%s | %s) ===\n" "$ARCH" "$KERNEL_REL"

# 1. Real-time kernel & boot isolation parameters (2:3:1 x86_64 vs 1:2:1 RK3326 aarch64)
if printf "%s" "$VERSION_STR" | grep -qiE "PREEMPT_RT|-rt"; then
  pass "Kernel RT" "PREEMPT_RT detected ($KERNEL_REL)"
else
  warn "Kernel RT" "PREEMPT_RT marker not seen in /proc/version ($KERNEL_REL)"
fi

case "$ARCH" in
  aarch64)
    if [[ -r /proc/device-tree/compatible ]] && tr '\0' ' ' < /proc/device-tree/compatible 2>/dev/null | grep -qiE "rk3326"; then
      for flag in "isolcpus=0,1,2" "nohz_full=0,1,2" "rcu_nocbs=0,1,2" "threadirqs"; do
        if [[ "$CMDLINE" == *"$flag"* ]]; then
          pass "RK3326 cmdline" "Found '$flag'"
        else
          fail "RK3326 cmdline" "Missing '$flag'"
        fi
      done
    else
      warn "aarch64 target" "Not RK3326 device tree; skipping strict 0,1,2 isolation check"
    fi
    ;;
  x86_64)
    for flag in "isolcpus=" "nohz_full=" "rcu_nocbs="; do
      if [[ "$CMDLINE" == *"$flag"* ]]; then
        pass "x86_64 cmdline" "Found '${flag%%=*}'"
      else
        warn "x86_64 cmdline" "Missing '${flag%%=*}' isolation flag"
      fi
    done
    ;;
  *)
    warn "Architecture" "Unsupported or unexpected arch '$ARCH'"
    ;;
esac

# 2. CPU governors & SMT/core layout
if [[ -r /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]]; then
  gov0="$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || true)"
  [[ "$gov0" == "performance" ]] && pass "CPU0 governor" "$gov0" || warn "CPU0 governor" "$gov0 (expected performance)"
fi

# 3. Aligned IRQ & threaded irq/<N>-* affinity check
check_irq_steering() {
  local arch="$1"
  local io_core_mask="8"
  local io_cpu_pin="3"

  if [[ "$arch" == "x86_64" ]]; then
    io_core_mask="20"
    io_cpu_pin="5"
  fi

  if [[ -r /proc/irq/default_smp_affinity ]]; then
    info "Default IRQ affinity" "$(tr -d ' \n' < /proc/irq/default_smp_affinity)"
  fi

  for irq_dir in /proc/irq/[0-9]*; do
    [[ -d "$irq_dir" ]] || continue
    local irq_num="${irq_dir##*/}"
    local actions_file="$irq_dir/actions"
    local aff_file="$irq_dir/smp_affinity"
    [[ -r "$actions_file" && -r "$aff_file" ]] || continue

    local actions aff_hex aff_dec
    actions="$(tr -d '\n' < "$actions_file" 2>/dev/null || true)"
    aff_hex="$(tr -d ' ,\n' < "$aff_file" 2>/dev/null || true)"
    [[ -n "$aff_hex" ]] || continue
    aff_dec="$((16#$aff_hex))" 2>/dev/null || aff_dec=0

    if printf "%s" "$actions" | grep -qiE '(^|[^a-z0-9])(xhci_hcd|dwc2|rk817|i2s|gpio|saradc)([^a-z0-9]|$)'; then
      if [[ $((aff_dec & 1)) -eq 1 ]]; then
        pass "RT/Audio IRQ $irq_num ($actions)" "mask 0x$aff_hex includes Core 0"
      else
        warn "RT/Audio IRQ $irq_num ($actions)" "mask 0x$aff_hex misses Core 0 (expected bit 0x01)"
      fi
      if command -v pgrep >/dev/null 2>&1 && command -v taskset >/dev/null 2>&1; then
        for tid in $(pgrep -f "^irq/${irq_num}-" 2>/dev/null || true); do
          local cur_aff
          cur_aff="$(taskset -p "$tid" 2>/dev/null | awk -F': ' '{print $2}' || true)"
          info "Threaded IRQ irq/${irq_num}" "PID $tid affinity $cur_aff (target Core 0)"
        done
      fi
    elif printf "%s" "$actions" | grep -qiE '(^|[^a-z0-9])(dwmmc|dw-mmc|mmc0|mmc1|nvme|ahci)([^a-z0-9]|$)'; then
      if [[ $((aff_dec & 1)) -eq 0 ]]; then
        pass "Storage IRQ $irq_num ($actions)" "mask 0x$aff_hex isolated from Core 0"
      else
        warn "Storage IRQ $irq_num ($actions)" "mask 0x$aff_hex hits Core 0 (expected Core $io_cpu_pin / 0x$io_core_mask)"
      fi
    fi
  done
}

check_irq_steering "$ARCH"

# 4. SCHED_FIFO priority 80 & HugeTLB / ZRAM limits
rtprio_limit="$(ulimit -r 2>/dev/null || echo 0)"
if [[ "$rtprio_limit" -ge 80 || "$(id -u)" -eq 0 ]]; then
  pass "SCHED_FIFO 80" "Permitted (ulimit -r: $rtprio_limit, uid: $(id -u))"
else
  fail "SCHED_FIFO 80" "Insufficient rtprio limit (ulimit -r: $rtprio_limit)"
fi

huge_total="$(awk '/HugePages_Total:/ {print $2}' /proc/meminfo 2>/dev/null || echo 0)"
huge_sz="$(awk '/Hugepagesize:/ {print $2}' /proc/meminfo 2>/dev/null || echo 0)"
if [[ "$huge_total" -gt 0 ]]; then
  pass "HugeTLB" "${huge_total} pages (${huge_sz} kB each)"
else
  warn "HugeTLB" "HugePages_Total is 0 (check vm.nr_hugepages or HugePage shim)"
fi

if [[ -e /dev/zram0 ]]; then
  zram_mb="$(( $(cat /sys/block/zram0/disksize 2>/dev/null || echo 0) / 1048576 ))"
  zram_comp="$(cat /sys/block/zram0/comp_algorithm 2>/dev/null || true)"
  swappiness="$(cat /proc/sys/vm/swappiness 2>/dev/null || echo n/a)"
  pass "ZRAM" "/dev/zram0 ${zram_mb} MB ($zram_comp), vm.swappiness=$swappiness"
else
  warn "ZRAM" "/dev/zram0 not found"
fi

# 5. Exclusive DAC lockfile (/run/lock/ax-dac.lock) & ALSA / mixer check (FC3 / RK817)
check_dac_and_lock() {
  local lock_file="/run/lock/ax-dac.lock"

  if [[ -e "$lock_file" ]]; then
    local pids=""
    if command -v fuser >/dev/null 2>&1; then
      pids="$(fuser "$lock_file" 2>/dev/null | tr -s ' ' || true)"
    fi
    if [[ -n "$pids" ]]; then
      info "DAC Lockfile" "$lock_file held by PID(s):$pids"
    else
      warn "DAC Lockfile" "$lock_file exists without active PID holder (stale lock)"
    fi
  else
    pass "DAC Lockfile" "Uncontended ($lock_file absent)"
  fi

  if [[ -d /proc/asound/FC3 ]] || grep -qiE "FC3|HiBy|ES9281" /proc/asound/cards 2>/dev/null; then
    pass "HiBy FC3 / ESS DAC" "Detected in ALSA"
    if command -v amixer >/dev/null 2>&1; then
      local card_id
      card_id="$(awk '/FC3|HiBy|ES9281/ {print $1; exit}' /proc/asound/cards 2>/dev/null || echo FC3)"
      amixer -c "$card_id" scontrols 2>/dev/null | sed 's/^/  [MIXER:FC3] /' || true
    fi
  else
    warn "HiBy FC3 / ESS DAC" "Not detected in /proc/asound/cards"
  fi

  if grep -qiE "rk817|rockchip" /proc/asound/cards 2>/dev/null; then
    pass "RK817 / Rockchip Codec" "Detected in ALSA"
    if command -v amixer >/dev/null 2>&1; then
      local rk_id
      rk_id="$(awk '/rk817|rockchip/ {print $1; exit}' /proc/asound/cards 2>/dev/null || true)"
      [[ -n "$rk_id" ]] && amixer -c "$rk_id" sget 'Playback' 2>/dev/null | awk '/Mono:|Front Left:/ {print "  [MIXER:RK817] " $0}' || true
    fi
  fi
}

check_dac_and_lock

# 6. Shared-memory telemetry presence
if [[ -e /dev/shm/ax_telemetry ]]; then
  pass "AX Telemetry" "/dev/shm/ax_telemetry present"
else
  info "AX Telemetry" "/dev/shm/ax_telemetry not yet active"
fi

printf "\nSummary: %d PASS, %d WARN, %d FAIL\n" "$PASS" "$WARN" "$FAIL"
[[ "$FAIL" -eq 0 ]] && exit 0 || exit 1
