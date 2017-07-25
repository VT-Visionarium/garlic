#!/bin/bash

# We need to start 8 servers on different DISPLAYS
# First, kill all existing vncservers on $SERVER
#ps -ef | grep vnc | grep -v 'grep' | awk '{print $2}' | xargs kill
#echo "Killed any running VNC servers."

START_VNCSERVER="vncserver -geometry 2560x1600"

set -ex
for i in {1..8}; do
	if $START_VNCSERVER :$i; then
		echo "VNC server successfully started on display port $i"
	else
		echo "Failed to start VNC server on display port $i"
	fi
done

