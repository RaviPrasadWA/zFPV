#!/bin/bash
# OpenHD Basic Binary Deployment Script
# Deploys OpenHD static binary to Pi Zero 2W

# --- CONFIGURATION ---
PI_USER="ravilinda"
PI_HOST="192.168.1.9"
PI_HOME=/home/$PI_USER


echo '📁 Setting up libs'
ssh $PI_USER@$PI_HOST "
echo '📦 Installing required camera packages...'
sudo apt update
sudo apt install -y libcamera-tools libcamera-dev libcamera-apps gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav raspberrypi-kernel-headers build-essential git dkms bc iw cmake

echo 'Libs setup complete.'
"


# --- COPY config.txt to Pi /boot ---
echo "Copying config.txt to Pi..."
scp config.txt $PI_USER@$PI_HOST:/tmp/config.txt

ssh $PI_USER@$PI_HOST "
sudo cp /boot/config.txt /boot/config.txt.bak 2>/dev/null || true
sudo cp /tmp/config.txt /boot/config.txt
sudo chown root:root /boot/config.txt
sudo chmod 644 /boot/config.txt
echo '✅ config.txt deployed to /boot/config.txt (backup at /boot/config.txt.bak)'
"

# --- Reboot Pi ---
ssh $PI_USER@$PI_HOST "sudo reboot"
