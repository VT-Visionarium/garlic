#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Enter the server on which to start VNCSERVER."
    exit 1
fi

SERVER=$1

# We need to start 8 servers on different DISPLAYS on $SERVER

# First, kill all existing vncservers on $SERVER
KILL_VNCSERVER="pkill -f vnc"
ssh $SERVER "$KILL_VNCSERVER"

# Export the path
EXPORT_PATH="export LD_LIBRARY_PATH=/usr/local/lib"
#ssh $SERVER "$EXPORT_PATH"

START_VNCSERVER="vncserver -geometry 2560x1600"
set -ex
for i in {1..8}; do
    if ssh $SERVER "$EXPORT_PATH && $START_VNCSERVER"; then
        echo "VNC server successfully started on $SERVER:$i"
    else
        echo "Failed to start VNC server on $SERVER:$i"
    fi
done
