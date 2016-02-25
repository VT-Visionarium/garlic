#!/bin/bash

# This script serves only to record how this encap package was installed.
# Running is just tells you how to install this encap thingy.

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD"
[ -f encap ] || exit 1
[ -f encap.pl ] || exit 1

sbin=/usr/local/sbin

if [ "$(id -u)" = 0 ] ; then
    set -x
    mkdir -p $sbin || exit 1
    cp -f $scriptdir/encap $scriptdir/encap.pl $sbin || exit 1
    chmod 755 $sbin/encap || exit 1
    chmod 644 $sbin/encap.pl || exit 1
    set +x
    echo SUCCESS
else

    echo "Look at this script and then run:"
    echo "sudo $0 $*"
    echo
    exit 1
fi

