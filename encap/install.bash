#!/bin/bash

# This script serves only to record how this encap package was installed.
# Running is just tells you how to install this encap thingy.

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD"
[ -f encap ] || exit 1
[ -f encap.pl ] || exit 1

sbin=/usr/local/sbin


cat << EOF || exit $?

Run as root:

 mkdir -p $sbin &&\
 cp -f $scriptdir/encap $scriptdir/encap.pl $sbin &&\
 chmod 755 $sbin/encap &&\
 chmod 644 $sbin/encap.pl &&\
 echo SUCCESS


EOF

