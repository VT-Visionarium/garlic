#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD"

function Fail()
{
    echo -e "\n$*\n"
    exit 1
}

# run as root or not at all
[ "$(id -u)" = 0 ] || Fail "You must run this as root"


# Usage: Check file inst_file
function Check()
{
	#vim -d $1 $2

	echo "------------------ $1 ---------------"

	diff $1 $2
}

Check login.defs /etc/login.defs
Check group.conf /etc/security/group.conf
Check nsswitch.conf /etc/nsswitch.conf
Check nslcd.conf /etc/nslcd.conf
Check ldap.conf /etc/openldap/ldap.conf

