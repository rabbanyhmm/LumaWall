#!/bin/bash

# endurance_test.sh
# Runs LumaWall for extended periods to detect memory leaks and stability issues.

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <video_path> <duration_hours>"
    exit 1
fi

VIDEO_PATH="$(realpath $1)"
HOURS="$2"
DURATION_MS=$(($HOURS * 3600 * 1000))

echo "Starting Endurance Test: $HOURS hours..."

cd "$(dirname "$0")/../.." || exit 1

# Valgrind is extremely slow with Vulkan/FFmpeg, so we use massif for memory profiling if needed
# For standard endurance, we just monitor memory usage from the outside using 'top' or 'ps'
xvfb-run ./build/src/app/lumawall --video "$VIDEO_PATH" --duration $DURATION_MS &
APP_PID=$!

echo "LumaWall running on PID $APP_PID..."

# Monitor loop
while kill -0 $APP_PID 2>/dev/null; do
    MEM=$(ps -o rss= -p $APP_PID)
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] Memory Usage: $((MEM/1024)) MB"
    sleep 600 # Log every 10 minutes
done

echo "Endurance test completed."
