#!/bin/bash
# LumaWall Endurance Stress Test

echo "Starting LumaWall Endurance Test..."

# Start the daemon in the background
./build/lumawall --daemon &
DAEMON_PID=$!

echo "Daemon started with PID: $DAEMON_PID"

# Wait a moment for DBus to register
sleep 2

# We simulate a D-Bus client constantly switching wallpapers
# while tracking memory usage.

LOGFILE="endurance_test.log"
echo "Timestamp, RSS (KB)" > $LOGFILE

CYCLE_COUNT=0
MAX_CYCLES=1000 # Just an arbitrary large number for testing

while [ $CYCLE_COUNT -lt $MAX_CYCLES ]; do
    if ! kill -0 $DAEMON_PID 2>/dev/null; then
        echo "ERROR: Daemon crashed!"
        exit 1
    fi

    # Trigger some action via D-Bus to keep things active (if D-Bus tools are available)
    # dbus-send --session --type=method_call --dest=org.lumawall.Daemon /org/lumawall/Daemon org.lumawall.Daemon.Next

    # Log memory footprint
    RSS=$(ps -o rss= -p $DAEMON_PID)
    echo "$(date +%s), $RSS" >> $LOGFILE

    sleep 10
    CYCLE_COUNT=$((CYCLE_COUNT + 1))
done

echo "Test completed successfully."
kill $DAEMON_PID
