#!/bin/bash
# Usage: ./push_to_pi.sh [pi_user@pi_host]

PI_ADDR=${1:-ravilinda@192.168.1.9}
PI_DIR=zFPV

ssh $PI_ADDR "
mkdir $PI_DIR
"

scp build_cmake.sh install_dep.sh CMakeLists.txt run.sh WBLib.cmake hardware.conf "$PI_ADDR:$PI_DIR/"
scp -r cmake executables lib src settings "$PI_ADDR:$PI_DIR/"