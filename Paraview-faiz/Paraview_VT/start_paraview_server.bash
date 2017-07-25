#!/bin/bash

# This assumes that you have ParaView 5.2.0 installled somewhere.
# In this file, I have hardcoded this to the path in my home directory. 
# Also, You need to give the correct path to the PVX file. I have again hard
# coded this for now to my own path.

if [ $# -ne 1 ]; then
	echo "Enter # of pvservers to start."
	echo "Example: start_pvserver.bash 8"
	exit 1
fi

# Kill any existing running pvservers
ps -ef | grep pvserver | grep -v grep | awk '{print $2}' | xargs kill  

if [ $# -ne 1 ]; then
	echo "Enter # of pvservers to start."
	echo "Example: start_pvserver.bash 8"
fi

SERVER=$1 

# Start new pvservers
mpiexec -np $1 \
	vglrun pvserver \
		cave-mono.pvx &

