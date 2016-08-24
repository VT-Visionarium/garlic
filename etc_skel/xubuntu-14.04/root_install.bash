#!/bin/bash

# this script does not do much, just copies
# some files into /etc/skel/

function Fail()
{
    echo -e "\n$*\n"
    exit 1
}


# run as root or not at all
[ "$(id -u)" = 0 ] || Fail "You must run this as root"

set -x
scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD"

host_skel="$(hostname -s)_skel" || Fail "hostname -s FAILED"


bakdir=

for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 ; do
    if [ ! -d /root/ORG_preSKEL_$i ] ; then
        bakdir=/root/ORG_preSKEL_$i
        break
    fi
done

[ -n "$bakdir" ] || Fail "Too many backup dirs"

mkdir $bakdir || exit $?
mv /etc/skel $bakdir || exit $?

cp -r skel /etc/ || exit $?
chmod -R a+r /etc/skel || exit $?
set +x


if [ -d "$host_skel" ] ; then
    for i in $host_skel/*.??* $host_skel/* ; do
        if [ -e "$i" ] ; then
            echo "cp -r $i /etc/skel"
            cp -r $i /etc/skel || Fail "cp -r $i /etc/skel FAILED"
        fi
    done
fi

echo "SUCCESS"
