#!/bin/bash

function Fail()
{
    echo -e "\n$*\n"
    exit 1
}


# run as root or not at all
[ "$(id -u)" = 0 ] || Fail "You must run this as root"

set -xe
apt-get update
apt-get dist-upgrade

mkdir /root/ORG_preLDAP
cp -a /etc /root/ORG_preLDAP
set +x

cat << EOF


  Okay so far.

  Do you need to reboot before going to the next script?
    
      sudo ./root_install_1.bash


EOF

