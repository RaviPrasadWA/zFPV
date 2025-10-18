#!/bin/bash
# Usage: ./run.sh [CHANNEL] [FREQ_MHZ] [TXPOWER_DBM]
CHANNEL=${1:-36}
FREQ=${2:-5180}
TXPOWER=${3:-15}

# Set your desired resolution and frame rate
WIDTH=1920
HEIGHT=1080
FPS=30

ip link set wlan1 down
iw dev wlan1 set type managed
ip link set wlan1 up
echo $((TXPOWER*100)) | tee /sys/module/88x2bu/parameters/openhd_override_tx_power_mbm
iw dev wlan1 set txpower fixed $((TXPOWER*100))

# Set up WiFi interface (wlan1)
ip link set wlan1 down
iw dev wlan1 set type monitor
ip link set wlan1 up
iw dev wlan1 set channel $CHANNEL
iw dev wlan1 set freq $FREQ


# Run libcamera-vid and pipe to Go transmitter
# libcamera-vid -t 0 --width $WIDTH --height $HEIGHT --framerate $FPS --codec h264 -o - | go run main.go
