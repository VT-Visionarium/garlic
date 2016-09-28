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

# It may be that these are not running yet, and may not be
# installed either, so failing here is fine.
#service nscd stop
#service nslcd stop
service sshd stop

yum install nss_ldap openldap-clients mlocate authconfig-gtk*

# installed nss-pam-ldapd and nscd (also need locate to find files)

# unlike debian (or ubuntu) adduser and useradd are the same program.

# now we have this
# nscd.service - Name Service Cache Daemon
# currently disabled.  Guess that means that the deamon is not running.
# also we have
# nslcd.service - Naming services LDAP client daemon.
# which is disabled too.


cp ldap.conf /etc/openldap/ldap.conf || exit $?
chmod 644 /etc/openldap/ldap.conf || exit $?

# wget the server's public key thingy with:
# wget --no-check-certificate\
# http://www.middleware.vt.edu/pubs/vt-cachain.pem


# ref: https://www.centos.org/forums/viewtopic.php?t=51004

cp vt-cachain.pem /etc/openldap/certs/ || exit $?
linkname="$(/etc/pki/tls/misc/c_hash vt-cachain.pem | awk '{print $1}')" || exit $?
#linkname="${linkname%%.0}"
ln -sf /etc/openldap/certs/vt-cachain.pem /etc/openldap/certs/${linkname} || exit $?
unset linkname


set +x

cat << EOF


  Okay now run some lib Open LDAP tests like:


  ldapsearch -H ldap://authn.directory.vt.edu\
 -x -Z -b ou=People,dc=vt,dc=edu '(uupid=fabidi89)'


  Not the ssh tests yet, that's later.
  And then continue by running (as root or not):

  
         system-config-authentication

  or

         authconfig-tui


  and


         sudo ./root_install_2.bash


EOF

