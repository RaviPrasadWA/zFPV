#!/usr/bin/env bash
# install_wblink_tx_service.sh
# Installs WB Link TX service if RX service is not already active.

set -euo pipefail

SERVICE_NAME="wb_link_tx"
OPPOSITE_SERVICE="wb_link_rx"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"

# 0️⃣ Check for opposite service
if systemctl list-unit-files | grep -q "^${OPPOSITE_SERVICE}.service"; then
  if systemctl is-active --quiet "${OPPOSITE_SERVICE}.service"; then
    echo "❌ Error: ${OPPOSITE_SERVICE}.service is already running. Cannot install ${SERVICE_NAME}."
    exit 1
  fi
  echo "⚠️ ${OPPOSITE_SERVICE}.service exists (disabled). Remove it before installing ${SERVICE_NAME}."
  exit 1
fi

# 1️⃣ Locate find_path.sh
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIND_SCRIPT="${SCRIPT_DIR}/find_path.sh"

if [[ ! -x "$FIND_SCRIPT" ]]; then
  echo "Error: find_path.sh not found or not executable in $SCRIPT_DIR"
  exit 1
fi

# 2️⃣ Run path detector
FOUND_PATH="$("$FIND_SCRIPT" | tr -d '[:space:]')"
if [[ -z "$FOUND_PATH" ]]; then
  echo "Error: could not determine default home path."
  exit 1
fi

# 3️⃣ Construct binary path
BINARY_PATH="${FOUND_PATH}/zFPV/build/wb_link_tx_impv"
if [[ ! -x "$BINARY_PATH" ]]; then
  echo "Error: binary not found or not executable: $BINARY_PATH"
  exit 1
fi

# 4️⃣ Get username
USER_NAME="$(stat -c '%U' "$FOUND_PATH" 2>/dev/null || echo pi)"

echo "Detected home path: $FOUND_PATH"
echo "Detected user: $USER_NAME"
echo "Binary path: $BINARY_PATH"

# 5️⃣ Create systemd service
sudo bash -c "cat > '$SERVICE_FILE' <<EOF
[Unit]
Description=WB Link TX Service
After=network.target

[Service]
Type=simple
User=root
ExecStart=/usr/bin/sudo ${BINARY_PATH}
WorkingDirectory=${FOUND_PATH}/zFPV/build
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF"

echo "✅ Created systemd service at: $SERVICE_FILE"

# 6️⃣ Reload + enable + start
sudo systemctl daemon-reload
sudo systemctl enable "${SERVICE_NAME}.service"
sudo systemctl restart "${SERVICE_NAME}.service"

# 7️⃣ Show status
sleep 1
sudo systemctl status "${SERVICE_NAME}.service" --no-pager