#!/bin/bash

# Because the NewRiver nodes are behind a firewall,
# we need to create a ssh tunnel to access and connect to them.

if [ $# -ne 2 ]; then
	echo "Enter the name of the NewRiver node that you got and your username."
	echo "Example: ./create_ssh_tunnel.bash nr025 fabidi89"
	exit 1
fi

NODE=$1
USERNAME=$2

ssh -L 5000:$NODE:11111 -L 5902:$NODE:5902 -L 5903:$NODE:5903 \
$USERNAME@newriver1.arc.vt.edu

