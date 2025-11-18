#!/usr/bin/env bash
# deduce-pi-default-home.sh
# Output: the best-guess default home directory for the machine (one path)
# Exit code 0 and prints path to stdout. If nothing found prints /root.

set -euo pipefail

# 1) If script was invoked with sudo, prefer SUDO_USER
if [[ -n "${SUDO_USER:-}" ]]; then
  # getent gives us a reliable passwd entry
  if getent passwd "$SUDO_USER" >/dev/null; then
    awk -F: -v u="$SUDO_USER" 'BEGIN{OFS=FS} $1==u{print $6; exit}' <(getent passwd) && exit 0
  fi
fi

# 2) If running as non-root interactive user (HOME set), prefer $HOME
if [[ -n "${HOME:-}" && "$(id -u)" -ge 1000 ]]; then
  echo "$HOME"
  exit 0
fi

# Helper: get UID_MIN from /etc/login.defs, fallback to 1000
UID_MIN=$(awk '/^[[:space:]]*UID_MIN/{print $2; exit}' /etc/login.defs 2>/dev/null || true)
if [[ -z "$UID_MIN" || ! "$UID_MIN" =~ ^[0-9]+$ ]]; then
  UID_MIN=1000
fi

# 3) If /home/pi exists (very common for Raspberry Pi images), return it first
if [[ -d /home/pi ]]; then
  echo "/home/pi"
  exit 0
fi

# 4) Find first passwd entry with uid >= UID_MIN and shell not /usr/sbin/nologin and valid home
while IFS=: read -r username passwd uid gid gecos home shell; do
  if [[ "$uid" -ge "$UID_MIN" && -n "$home" && "$home" != "/" && -d "$home" ]]; then
    # skip nobody or nologin-like accounts
    if [[ "$username" == "nobody" ]]; then
      continue
    fi
    if [[ "$shell" == "/usr/sbin/nologin" || "$shell" == "/bin/false" ]]; then
      continue
    fi
    echo "$home"
    exit 0
  fi
done < /etc/passwd

# 5) As a last resort: try user 'pi' explicitly or fallback to /root
if [[ -d "/home/pi" ]]; then
  echo "/home/pi"
else
  echo "/root"
fi

exit 0
