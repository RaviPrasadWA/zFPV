# bin/bash
set -e

# Get CPU cores and available RAM (in MB)
CORES=$(nproc)
MEM_MB=$(grep MemAvailable /proc/meminfo | awk '{print int($2/1024)}')

# Assume each compile job needs ~200 MB
JOBS=$(( MEM_MB / 200 ))
if [ "$JOBS" -gt "$CORES" ]; then
  JOBS=$CORES
elif [ "$JOBS" -lt 1 ]; then
  JOBS=1
fi
echo "Detected ${CORES} cores, ${MEM_MB} MB RAM → using -j${JOBS}"

# Start time to measure the time taken for compilation in seconds
start=$(date +%s)


# convenient script to build this project with cmake
rm -rf build

mkdir build

cd build

cmake .. -DENABLE_USB_CAMERAS=OFF

make -j${JOBS}

end=$(date +%s)
runtime=$((end - start))

echo "✅ Build completed in ${runtime} seconds."