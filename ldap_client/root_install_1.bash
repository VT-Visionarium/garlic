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

set -x
# at this point we have just updated with apt-get dist-upgrade
export DEBIAN_FRONTEND=noninteractive
# cross your fingers that the distro have not broken this yet.
# This will install:
# nslcd ldap-utils libnss-ldapd libpam-ldapd nscd
apt-get -y install nslcd || exit $?

cp ldap.conf /etc/ldap/ldap.conf || exit $?
chmod 644 /etc/ldap/ldap.conf || exit $?


set +x

cat << EOF


  Okay now run some lib Open LDAP tests like:


  ldapsearch -H ldap://authn.directory.vt.edu\
 -x -Z -b ou=People,dc=vt,dc=edu '(uupid=fabidi89)'


  Not the ssh tests yet, that's later.
  And then continue by running 
  
      sudo ./root_install_2.bash

EOF

