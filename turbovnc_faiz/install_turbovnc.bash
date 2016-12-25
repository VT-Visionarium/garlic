#!/bin/bash

# To install TurboVNC, we need to install some dependencies first
# The first one is libjpeg-turbo SDK - https://github.com/libjpeg-turbo/libjpeg-turbo
# You can install that by running the other script called
# install_libjpeg_turbo.bash

# Go to the location from where you want to run this script
cd /home/faiz89/git

# Need to install pam for TurboVNC server
set -ex
echo "Checking if pam-devel already installed..."
sudo yum install pam-devel -y
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
# Make a build directory
mkdir build01 && cd build01
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
cmake -G"Unix Makefiles"\
    -DCMAKE_INSTALL_PREFIX=/usr/local/encap/turbovnc-v2.1.1 \
    -DTJPEG_INCLUDE_DIR=/usr/local/include \
    -DTJPEG_JAR=/usr/local/share/classes/turbojpeg.jar \
    -DTJPEG_JNILIBRARY=/usr/local/lib/libturbojpeg.so \
    ../../turbovnc
# Set encoding to UTF8 else the make may fail complaining of non-ASCII
# characters
export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8
# Make the file
make -j 4
set +ex

# Finally, install turbovnc here
make install

