#!/bin/bash

echo "Run this script from the login node on NewRiver."

if [ $# -ne 3 ]; then
	echo "Enter walltime, # nodes, and email address."
	echo "Example: ./request_node.bash 4 1 fabidi89@vt.edu"
	exit 1
fi

WALLTIME=$1
NODES=$2
EMAIL=$3

qsub -I -l walltime=$1:00:00 -l nodes=$2:ppn=24:gpus=2 -q vis_q -W \
group_list=newriver -A arctest -M $3 -m bea

