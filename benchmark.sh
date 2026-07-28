#!/bin/bash
set -e
unset WAYLAND_DISPLAY

echo "=== LumaWall X11 Platform Benchmark ==="

echo ""
echo "[Startup Time & Memory]"
/usr/bin/time -v xvfb-run -s "-screen 0 1920x1080x24" ./build_release/src/app/lumawall

echo ""
echo "[Thread Count]"
xvfb-run -s "-screen 0 1920x1080x24" ./build_release/src/app/lumawall --idle &
PID=$!
sleep 1
THREADS=$(cat /proc/$PID/status | grep Threads | awk '{print $2}')
echo "Active threads: $THREADS"
kill -9 $PID 2>/dev/null || true
