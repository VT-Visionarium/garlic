#!/bin/bash

# on anvil ran:

if [ "$(id -u)" != "0" ] ; then
    echo "Run this $0 as root"
    exit 1
fi

set -x
apt-get install mysql-server


