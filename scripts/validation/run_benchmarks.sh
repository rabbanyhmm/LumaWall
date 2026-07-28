#!/bin/bash

# run_benchmarks.sh
# Runs LumaWall in benchmark mode on a provided video file and reports compatibility.

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <video_path>"
    exit 1
fi

VIDEO_PATH="$(realpath $1)"
DURATION_MS=10000

echo "Running benchmarks on $VIDEO_PATH for ${DURATION_MS}ms..."
cd "$(dirname "$0")/../.." || exit 1
xvfb-run ./build/src/app/lumawall --video "$VIDEO_PATH" --benchmark --duration $DURATION_MS
