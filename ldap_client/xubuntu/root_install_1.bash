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

# It may be that these are not running yet, and may not be
# installed either, so failing here is fine.
service nscd stop
service nslcd stop
service ssh stop


# ubuntu bug work-around
# The startup of nslcd gave this error:
# nslcd: Warning: /lib/x86_64-linux-gnu/libnss_ldap.so.2: undefined symbol: _nss_ldap_enablelookups (probably older NSS module loaded)
# Remove broken libnss_ldap.so; or not, if it's not there.
# Why? Because this worked.  Installing a new version of nslcd does not
# seem to remove this broken version of it.
apt-get purge libnss-ldapd
# Why did purge not remove this?
rm -f /lib/x86_64-linux-gnu/libnss_ldap*

set +x
echo
echo "It's okay if some of the last 5 commands failed."
echo
set -x

# This will install:
# nslcd ldap-utils libnss-ldapd libpam-ldapd nscd
apt-get -y install nslcd libnss-ldapd || exit $?

cp ldap.conf /etc/ldap/ldap.conf || exit $?
chmod 644 /etc/ldap/ldap.conf || exit $?


set +x

cat << EOF


  Okay now run some lib Open LDAP tests like:


  ldapsearch -H ldap://login.directory.vt.edu\
 -x -Z -b ou=People,dc=vt,dc=edu '(uupid=fabidi89)'


  Not the ssh tests yet, that's later.
  And then continue by running 
  
      sudo ./root_install_2.bash

EOF

