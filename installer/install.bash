#!/bin/bash

# 'hostname -s' is the name of the file list of projects to install

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path

function Fail()
{
    [ -n "$1" ] && echo -e "$*"
    echo
    echo "running script $0 FAILED"
    echo
    exit 1
}


host="$(hostname -s)" || exit $?
[ -n "$host" ] || exit 1
hostfile="$scriptdir/$host"
[ -f "$hostfile" ] ||\
    Fail "FILE LIST for host $host ($hostfile) was not found\n\
\n\
  Copy an existing one from $scriptdir to $hostfile and edit it."

set +x
echo
echo "Running install scripts from $hostfile"
echo

source "$hostfile" || Fail

set +x
echo
echo "-------------- SUCCESS --------------------------"
echo

