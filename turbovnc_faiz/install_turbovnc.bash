#!/bin/bash

# To install TurboVNC, we need to install some dependencies first
# The first one is libjpeg-turbo SDK - https://github.com/libjpeg-turbo/libjpeg-turbo
# You can install that by running the other script called
# install_libjpeg_turbo.bash

# Check operating system first
if [ -f /etc/lsb-release ]; then
    . /etc/lsb-release
    DISTRO=$DISTRIB_ID
elif [ -f /etc/debian_version ]; then
    DISTRO=Debian
    # XXX or Ubuntu
elif [ -f /etc/redhat-release ]; then
    DISTRO="Red Hat"
    # XXX or CentOS or Fedora
else
    DISTRO=$(uname -s)
fi

# Go to the location from where you want to run this script
cd /home/faiz89/git

# Need to install pam for TurboVNC server
set -ex
echo "Checking if pam already installed..."
if [ "$DISTRO" == "Red Hat" ]; then
    sudo yum install pam-devel -y
else
    sudo apt-get install libpam0g-dev -y
fi
set +ex

# Check if there already is a git repo downloaded in ~/git
TURBOVNC="$(ls /home/faiz89/git | grep turbovnc)"
if [ -z "$TURBOVNC" ]; then
    set -ex
    echo "Cloning turbovnc from GitHub..."
    # Clone the turbovnc repo from GitHub
    git clone https://github.com/TurboVNC/turbovnc.git
    set +ex
fi

set -ex
# Building turbovnc
# Go to the source files
cd turbovnc

# Make a build directory if not exists
if [ ! -d /home/faiz89/git/turbovnc/build01 ]; then
    mkdir build01 && cd build01
else
    cd build01
fi

# Find the path where turbojpeg.h is located
# Assuming it is in /usr/local/include/turbojpeg.h
TURBOJPEG="/usr/local/include"

# Install it in /usr/local/encap/turbovnc-v2.1.1
# Create folder if not exists
if [ ! -d /usr/local/encap/turbovnc-v2.1.1 ]; then
    sudo mkdir /usr/local/encap/turbovnc-v2.1.1
    # Change file permissions
    sudo chown -R faiz89:faiz89 /usr/local/encap/turbovnc-v2.1.1
fi

# Configure
cmake -G"Unix Makefiles"\
    -DCMAKE_INSTALL_PREFIX=/usr/local/encap/turbovnc-v2.1.1 \
    -DTJPEG_INCLUDE_DIR=/usr/local/include \
    -DTJPEG_JAR=/usr/local/share/classes/turbojpeg.jar \
    -DTJPEG_JNILIBRARY=/usr/local/lib/libturbojpeg.so \
    -DTVNC_USETLS=0 \
    ../../turbovnc

# Set encoding to UTF8 else the make may fail complaining of non-ASCII
# characters
export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8

# Make the file
make -j 4
set +ex

# Finally, install turbovnc here
make install

echo "Now run sudo encap to install it using the encap project."

