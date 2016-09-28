#!/bin/bash

function Fail()
{
    echo -e "\n$*\n"
    exit 1
}

# run as root or not at all
[ "$(id -u)" = 0 ] || Fail "You must run this as root"

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD"

function Prompt()
{
    local a
    set +x
    echo
    echo "    < Cntl-C > to quit"
    if [ -n "$1" ] ; then
        echo -ne "\n\n$*\n ===< enter to continue > ===>>>"
    else
        echo -ne "\n\n ===< enter to continue > ===>>>"
    fi
    read a
}


Prompt "Note this script (2) is interactive, it steps through things"

set -x


######################## edit /etc/login.defs ##########################

set +x
Prompt "NEXT we vim edit /etc/login.defs to change the default UMASK to 022"

set -x
vim /etc/login.defs || exit $?

###################### change /etc/security/group.conf" ######################

set +x
Prompt "replace /etc/security/group.conf"
set -x
cp group.conf /etc/security/group.conf || exit $?
chmod 644 /etc/security/group.conf || exit $?



set +x
Prompt "NEXT this script will Restart ssh, nscd, and nslcd"

set -x
groupadd -r nslcd
service sshd restart || exit $?
service nscd restart || exit $?
service nslcd restart || exit $?

set +x


cat << EOF

  Now run some ssh tests with a user that have an LDAP account
  both with and without a local home directory.

  start and stop nscd and nslcd via 'service nscd start' and so on.

  if it all works leave the services ssh, nscd, and nslcd running

      try:

          id ambrown7



  that's it.

EOF

