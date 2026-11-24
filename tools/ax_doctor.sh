# ------------------------------------------------------------------------------
# Tuned IRQ Affinity Check (Core 0 Audio/USB vs Core 3/5 Storage/Housekeeping)
# ------------------------------------------------------------------------------
check_irq_steering() {
  local arch="$1"
  local rt_core_mask="1"
  local io_core_mask="8"
  local io_cpu_pin="3"

  if [[ "$arch" == "x86_64" ]]; then
    io_core_mask="20"
    io_cpu_pin="5"
  fi

  if [[ -r /proc/irq/default_smp_affinity ]]; then
    info "Default IRQ affinity" "$(cat /proc/irq/default_smp_affinity)"
  fi

  for irq_dir in /proc/irq/[0-9]*; do
    [[ -d "$irq_dir" ]] || continue
    local irq_num="${irq_dir##*/}"
    local actions_file="$irq_dir/actions"
    local aff_file="$irq_dir/smp_affinity"
    [[ -r "$actions_file" && -r "$aff_file" ]] || continue

    local actions aff_hex aff_dec
    actions="$(tr -d '\n' < "$actions_file" 2>/dev/null || true)"
    aff_hex="$(tr -d ' \n' < "$aff_file" 2>/dev/null || true)"
    aff_dec="$((16#$aff_hex))" 2>/dev/null || aff_dec=0

    if printf "%s" "$actions" | grep -qiE '(^|[^a-z0-9])(xhci_hcd|dwc2|rk817|i2s|gpio|saradc)([^a-z0-9]|$)'; then
      if [[ $((aff_dec & 1)) -eq 1 ]]; then
        pass "RT/Audio IRQ $irq_num ($actions)" "mask 0x$aff_hex includes Core 0"
      else
        warn "RT/Audio IRQ $irq_num ($actions)" "mask 0x$aff_hex misses Core 0 (expected bit 0x01)"
      fi
      if command -v taskset >/dev/null 2>&1; then
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

# ------------------------------------------------------------------------------
# Tuned DAC Lockfile, ALSA Device & Mixer Check (FC3 / RK817)
# ------------------------------------------------------------------------------
check_dac_and_lock() {
  local lock_file="/run/lock/ax-dac.lock"

  if [[ -e "$lock_file" ]]; then
    local pids=""
    if command -v fuser >/dev/null 2>&1; then
      pids="$(fuser "$lock_file" 2>/dev/null | tr -s ' ' || true)"
    fi
    if [[ -n "$pids" ]]; then
      info "DAC Lockfile (/run/lock/ax-dac.lock)" "Active holder PID(s):$pids"
    else
      warn "DAC Lockfile (/run/lock/ax-dac.lock)" "File exists with no active PID holder (check for stale lock)"
    fi
  else
    pass "DAC Lockfile (/run/lock/ax-dac.lock)" "Uncontended (no active lockfile)"
  fi

  if [[ -d /proc/asound/FC3 ]] || grep -qiE "FC3|HiBy|ES9281" /proc/asound/cards 2>/dev/null; then
    pass "HiBy FC3 / ESS DAC" "Present in ALSA (/proc/asound/cards)"
    if command -v amixer >/dev/null 2>&1; then
      local card_id
      card_id="$(awk '/FC3|HiBy|ES9281/ {print $1; exit}' /proc/asound/cards 2>/dev/null || echo FC3)"
      amixer -c "$card_id" scontrols 2>/dev/null | sed 's/^/  [MIXER] /' || true
    fi
  else
    warn "HiBy FC3 / ESS DAC" "Not detected in /proc/asound/cards"
  fi

  if grep -qiE "rk817|rockchip" /proc/asound/cards 2>/dev/null; then
    pass "RK817 / Rockchip Codec" "Present in ALSA"
    if command -v amixer >/dev/null 2>&1; then
      local rk_id
      rk_id="$(awk '/rk817|rockchip/ {print $1; exit}' /proc/asound/cards 2>/dev/null || true)"
      [[ -n "$rk_id" ]] && amixer -c "$rk_id" sget 'Playback' 2>/dev/null | awk '/Mono:|Front Left:/ {print "  [RK817] " $0}' || true
    fi
  fi
}
