#!/bin/bash

# stress_test.sh
# Runs a stress test on LumaWall by repeatedly initializing and terminating playback.

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <video_path>"
    exit 1
fi

VIDEO_PATH="$(realpath $1)"
ITERATIONS=10
DURATION_MS=2000

echo "Starting Stress Test: Rapid Initialization ($ITERATIONS iterations)..."
cd "$(dirname "$0")/../.." || exit 1

for i in $(seq 1 $ITERATIONS); do
    echo "Iteration $i / $ITERATIONS..."
    xvfb-run ./build/src/app/lumawall --video "$VIDEO_PATH" --duration $DURATION_MS > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: LumaWall crashed on iteration $i!"
        exit 1
    fi
done

echo "Stress test passed successfully."
