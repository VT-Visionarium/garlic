#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Enter username."
	echo "Example: ./ssh_newriver.bash fabidi89"
	exit 1
fi

USERNAME=$1

ssh $USERNAME@newriver1.arc.vt.edu

