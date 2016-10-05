#!/bin/bash

# I have no idea if this script will work;
# so think of it as just notes.
exit 1

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

set -x

# alias cp='cp -i' is a pain.
unalias cp
unalias ln

yum install nss_ldap openldap-clients
# installs nss-pam-ldapd openldap-clients nscd

# running system-config-authentication has helped.


mkdir /root/ORG_preLDAP || exit $?
cp -a /etc /root/ORG_preLDAP || exit $?

service sshd stop
service nslcd stop
service nscd stop

# wget the server's public key thingy with:
# wget --no-check-certificate\
# http://www.middleware.vt.edu/pubs/vt-cachain.pem
# We checked in a copy into the github repo
# that may be okay still.

cp vt-cachain.pem /etc/openldap/certs/ || exit $?
linkname="$(/etc/pki/tls/misc/c_hash vt-cachain.pem | awk '{print $1}')" || exit $?
#linkname="${linkname%%.0}"
ln -sf /etc/openldap/certs/vt-cachain.pem /etc/openldap/certs/${linkname} || exit $?
unset linkname


set +x


# Usage: Cp file toPath mode
# interactive file replacer
# so you may see the diff you are making
function Cp()
{
	[ -z "$3" ] && exit 1
	set -x	
	cp $1 $2 || exit 1
	chmod $3 $2 || exit 1
	set +x
}

Cp login.defs /etc/login.defs 644
Cp group.conf /etc/security/group.conf 644
Cp nsswitch.conf /etc/nsswitch.conf 644
Cp nslcd.conf /etc/nslcd.conf 600
Cp ldap.conf /etc/openldap/ldap.conf 644

set -x



# pick one:

cp pam.d/*-ac /etc/pam.d/
# cp GNOME_pam.d/*-ac /etc/pam.d/




groupadd -r nslcd
service nslcd start
service nscd start
service sshd start




# ref: http://blog.zwiegnet.com/linux-server/configure-centos-7-ldap-client/
# making GNOME desktop manager
# If using gdm (GNOME) edit /etc/sysconfig/authconfig
# and make the line with FORCELEGACY be
FORCELEGACY=yes

reboot, and re-run authconfig-tui

