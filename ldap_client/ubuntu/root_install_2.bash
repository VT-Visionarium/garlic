#!/bin/bash

function Fail()
{
    echo -e "\n$*\n"
    exit 1
}

# run as root or not at all
[ "$(id -u)" = 0 ] || Fail "You must run this as root"

set -e

scriptdir="$(dirname ${BASH_SOURCE[0]})"
cd "$scriptdir"
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
 http://www.middleware.vt.edu/pubs/vt-cachain.pem
fi

cp vt-cachain.pem /etc/ca-certificates/vt-cachain.pem
chmod 644 /etc/ca-certificates/vt-cachain.pem

cp nsswitch.conf /etc/nsswitch.conf
chmod 644 /etc/nsswitch.conf

cp nslcd.conf /etc/nslcd.conf
chmod 640 /etc/nslcd.conf

cp adduser.conf /etc/adduser.conf
chmod 640 /etc/adduser.conf


apt-get install ssh

date="$(date)"

set +x
Prompt "NEXT We'll edit /etc/ssh/sshd_config with vim and add a AllowUsers lance ..."

set -x

cat << EOF >> /etc/ssh/sshd_config

# Added by lance on $date
AllowUsers lance lanceman npolys faiz89 fabidi89
EOF

vim /etc/ssh/sshd_config


######################## edit /etc/pam.d/common-session ##########################

set +x
Prompt "NEXT with add/check with vim pam_mkhomedir.so (bottom) to /etc/pam.d/common-session"

cat << EOF >> /etc/pam.d/common-session

# lance added next line $date
session required    pam_mkhomedir.so  skel=/etc/skel  umask=0022
EOF

vim /etc/pam.d/common-session

set +x
echo "/etc/pam.d/common-session has been changed"
echo

######################## edit /etc/pam.d/common-auth ##########################

set +x
Prompt "NEXT with add/check with vim adding pam_group.so to /etc/pam.d/common-auth"

set -x
tmp=$(mktemp --suffix=ldap_client_pam_common_)
cat << EOF > $tmp
# next line added by lance on $date
auth    required    pam_group.so use_first_pass

EOF
cat /etc/pam.d/common-auth >> $tmp
vim $tmp

Prompt "SAVE this edit as /etc/pam.d/common-auth"
set -x
mv $tmp /etc/pam.d/common-auth
chmod 644 /etc/pam.d/common-auth

####################### change /etc/security/group.conf" ######################

set +x
Prompt "replace /etc/security/group.conf"
set -x
cp group.conf /etc/security/group.conf
chmod 644 /etc/security/group.conf


######################## edit /etc/login.defs ##########################

set +x
Prompt "NEXT we vim edit /etc/login.defs to change the default UMASK to 022"

set -x
vim +152 /etc/login.defs





set +x
Prompt "NEXT this script will Restart ssh, nscd, and nslcd"

set -x
service ssh restart
service nscd restart
service nslcd restart

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

