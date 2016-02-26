#!/bin/bash

# Do not source ../common.bash with this too.

# 'hostname -s' is the name of the file list of projects to install


topsrc=/usr/local/src
srcdir=/usr/local/src/installer
cwd="$PWD"
progs[0]=

function Fail()
{
    [ -n "$1" ] && echo "$*"
    echo
    echo "running script $0 from $cwd FAILED"
    echo
    exit 1
}


function InstallPackages()
{
    [ -n "$1" ] || Fail "Usage: ${FUNCNAME[0]} LIST_FILE"
    local i

    # keep the pipe line program in the same shell context
    # so that progs[] and i get set in this shell context
    shopt -s lastpipe

    grep -v -e '^ *#\|^ *$' "$1" |\
        while read -r line ; do
            progs[$i]="$line"
            let i=$i+1
        done
    progs[$i]=
    i=0
    while [ -n "${progs[$i]}" ] ; do
        echo "Running: ${progs[$i]}"
        if ! ${progs[$i]} ; then
            Fail "Running: ${progs[$i]} failed"
        fi
        let i=$i+1
    done

    echo
    echo "----------- SUCCESSFULLY INSTALLED --------------"
    echo
    cat $1
    echo
    echo "-------------- SUCCESS --------------------------"
    echo
}


host="$(hostname -s)" || exit 1
[ -n "$host" ] || exit 1
hostfile="$srcdir/$host"
[ -f "$hostfile" ] ||\
    Fail "FILE LIST for host $host ($hostfile) was not found"

echo
echo "Running install scripts from $hostfile"
echo

cd "$topsrc" || Fail

InstallPackages "$hostfile"

