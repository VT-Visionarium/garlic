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

if [ ! -f vt-cachain.pem ] ; then
    # wget the server's public key thingy with:
    wget --no-check-certificate\
 http://www.middleware.vt.edu/pubs/vt-cachain.pema || Fail
fi

cp vt-cachain.pem /etc/ca-certificates/vt-cachain.pem || exit $?
chmod 644 /etc/ca-certificates/vt-cachain.pem || exit $?

cp nsswitch.conf /etc/nsswitch.conf || exit $?
chmod 644 /etc/nsswitch.conf || exit $?

cp nslcd.conf /etc/nslcd.conf || exit $?
chmod 640 /etc/nslcd.conf || exit $?

cp adduser.conf /etc/adduser.conf || exit $?
chmod 640 /etc/adduser.conf || exit $?


apt-get install ssh || exit $?

date="$(date)" || exit $?

set +x
Prompt "NEXT We'll edit /etc/ssh/sshd_config with vim and add a AllowUsers lance ..."

set -x

cat << EOF >> /etc/ssh/sshd_config || exit $?

# Added by lance on $date
AllowUsers lance lanceman npolys faiz89 fabidi89
EOF

vim /etc/ssh/sshd_config || exit $?


set +x
Prompt "NEXT with add/check with vim  pam_mkhomedir.so to /etc/pam.d/common-session"

set -x
cat << EOF >> /etc/pam.d/common-session || exit $?

# lance added next line $date
session required    pam_mkhomedir.so  skel=/etc/skel  umask=0022
EOF

vim /etc/pam.d/common-session || exit $?


set +x
Prompt "NEXT we vim edit /etc/login.defs to change the default UMASK to 022"

set -x
vim +152 /etc/login.defs || exit $?

set +x
Prompt "NEXT this script will Restart ssh, nscd, and nslcd"

set -x
service ssh restart || exit $?
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


      that's it...

EOF

