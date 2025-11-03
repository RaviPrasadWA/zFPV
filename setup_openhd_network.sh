#!/usr/bin/env bash
#
# setup_vtx_air_network.sh
#
# Prepare a Pi (VTX/Air) so that the T3U adapter (default: wlan1) is reserved for OpenHD
# and NOT managed by NetworkManager/dhcpcd/wpa_supplicant, while keeping wlan0 for SSH/networking.
#
# - Does NOT start NetworkManager. If NM is active, it is reloaded only to apply unmanaged rules.
# - Idempotent: safe to run multiple times.
# - Optional: set regulatory domain with --country US (requires `iw`).
#
# Usage:
#   sudo ./setup_vtx_air_network.sh [--iface wlan1] [--country US]
#
set -euo pipefail

IFACE="wlan1"
COUNTRY=""

usage() {
  cat <<USAGE
Usage: $0 [--iface IFACE] [--country CC]

Options:
  -i, --iface IFACE   Interface to reserve for OpenHD (default: wlan1)
  -c, --country CC    Regulatory domain to set (e.g., US, EU). Requires 'iw'. Optional.
  -h, --help          Show this help

Behavior:
  - Marks IFACE unmanaged for NetworkManager (by MAC) via conf.d, if present (no automatic start)
  - Adds 'denyinterfaces IFACE' to /etc/dhcpcd.conf and releases DHCP on IFACE
  - Stops wpa_supplicant for IFACE (leaves other ifaces like wlan0 alone)
  - Flushes all IPs from IFACE and brings it down (OpenHD will take over and switch to monitor mode)
  - Prints pre-/post-state summaries
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -i|--iface)
      IFACE=${2:-}
      shift 2
      ;;
    -c|--country)
      COUNTRY=${2:-}
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

# Elevate to root if needed
if [[ ${EUID:-$(id -u)} -ne 0 ]]; then
  exec sudo -E bash "$0" ${IFACE:+--iface "$IFACE"} ${COUNTRY:+--country "$COUNTRY"}
fi

# Validate interface exists
if [[ ! -e "/sys/class/net/$IFACE" ]]; then
  echo "Error: Interface '$IFACE' not found." >&2
  echo "Available interfaces:" >&2
  ip -o link show | awk -F': ' '{print " - "$2}' >&2
  exit 1
fi

MAC=$(cat "/sys/class/net/$IFACE/address" || true)
DRV=$(basename "$(readlink -f "/sys/class/net/$IFACE/device/driver" 2>/dev/null || echo /unknown)" || echo unknown)

echo "=== Pre-state ==="
echo "Host: $(hostname)  Kernel: $(uname -r)  IFACE: $IFACE  MAC: $MAC  DRIVER: $DRV"
if command -v systemctl >/dev/null 2>&1; then
  NM_STATE=$(systemctl is-active NetworkManager 2>/dev/null || true)
  echo "NetworkManager: ${NM_STATE:-unknown}"
  if [[ "$NM_STATE" == "active" ]] && command -v nmcli >/dev/null 2>&1; then
    nmcli -t -f DEVICE,STATE,CONNECTION device status || true
  fi
fi
ip -4 -o addr show dev "$IFACE" || echo "no IPv4 on $IFACE"
ip -6 -o addr show dev "$IFACE" || echo "no IPv6 on $IFACE"
echo -n "operstate: "; cat "/sys/class/net/$IFACE/operstate" || true
pgrep -a wpa_supplicant || true
pgrep -a dhcpcd || true

echo
echo "=== Applying configuration to free $IFACE for OpenHD ==="

# 1) Persist: tell NetworkManager (if used later) to ignore this MAC
mkdir -p /etc/NetworkManager/conf.d
cat > /etc/NetworkManager/conf.d/unmanaged-openhd.conf <<EOF
[keyfile]
unmanaged-devices=mac:${MAC}
EOF
echo "Wrote /etc/NetworkManager/conf.d/unmanaged-openhd.conf (MAC: $MAC)"

# Reload NM only if active; do not start it
if command -v systemctl >/dev/null 2>&1 && systemctl is-active --quiet NetworkManager; then
  systemctl reload NetworkManager || true
fi

# 2) Persist: ask dhcpcd to ignore IFACE
touch /etc/dhcpcd.conf
if ! grep -qE "^\s*denyinterfaces\s+$IFACE\b" /etc/dhcpcd.conf; then
  echo "denyinterfaces $IFACE" >> /etc/dhcpcd.conf
  echo "Appended 'denyinterfaces $IFACE' to /etc/dhcpcd.conf"
else
  echo "denyinterfaces $IFACE already present in /etc/dhcpcd.conf"
fi

# 3) Stop dhcpcd management on IFACE now (no service restart)
dhcpcd -x "$IFACE" 2>/dev/null || true

# 4) Stop wpa_supplicant only for IFACE (leave others running)
if systemctl list-unit-files | grep -q '^wpa_supplicant@\.service'; then
  systemctl is-active --quiet "wpa_supplicant@$IFACE" && systemctl stop "wpa_supplicant@$IFACE" || true
fi
PIDS=$(pgrep -f "wpa_supplicant.*-i$IFACE" || true)
if [[ -n "${PIDS:-}" ]]; then
  echo "Killing wpa_supplicant for $IFACE: $PIDS"
  kill $PIDS || true
  sleep 1
fi

# 5) Remove any IP addresses from IFACE and bring it down
ip addr flush dev "$IFACE" || true
ip link set dev "$IFACE" down || true

# Optional: set regulatory domain if requested
if [[ -n "$COUNTRY" ]]; then
  if command -v iw >/dev/null 2>&1; then
    echo "Setting regulatory domain to $COUNTRY"
    iw reg set "$COUNTRY" || true
  else
    echo "Note: 'iw' not installed; skipping regulatory domain set for $COUNTRY"
  fi
fi

echo
echo "=== Post-state ==="
if command -v systemctl >/dev/null 2>&1; then
  NM_STATE=$(systemctl is-active NetworkManager 2>/dev/null || true)
  echo "NetworkManager: ${NM_STATE:-unknown}"
  if [[ "$NM_STATE" == "active" ]] && command -v nmcli >/dev/null 2>&1; then
    nmcli -t -f DEVICE,STATE,CONNECTION device status || true
  fi
fi
ip -4 -o addr show dev "$IFACE" || echo "no IPv4 on $IFACE"
ip -6 -o addr show dev "$IFACE" || echo "no IPv6 on $IFACE"
echo -n "operstate: "; cat "/sys/class/net/$IFACE/operstate" || true
pgrep -a wpa_supplicant || true
pgrep -a dhcpcd || true

echo
echo "Done. $IFACE is now free for OpenHD (no IP, unmanaged, link down)."
echo "Your wlan0 (or other interface) remains available for SSH/networking."
