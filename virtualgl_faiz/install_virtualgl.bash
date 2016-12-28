#!/bin/bash

# To install VirtualGL, we need to install some dependencies first
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

# Need to install freeglut library
if [ $DISTRO == "Red Hat" ]; then
    sudo yum install mesa-libGL-devel mesa-libGLU-devel
else
    sudo apt-get install libglu-dev
fi

# Go to the location from where you want to run this script
cd /home/faiz89/git

# We are not installing VirtualGL with SSL support.
# Hence, no need to install openssl

# Check if there already is a git repo downloaded in ~/git
VIRTUALGL="$(ls /home/faiz89/git | grep virtualgl)"
if [ -z "$VIRTUALGL" ]; then
    set -ex
    echo "Cloning virtualgl from GitHub..."
    # Clone the virtualgl repo from GitHub
    git clone https://github.com/VirtualGL/virtualgl.git
    set +ex
fi

set -ex
# Building virtualgl
# Go to the source files
cd virtualgl

# Make a build directory if not exists
if [ ! -d /home/faiz89/git/virtualgl/build01 ]; then
    mkdir build01 && cd build01
else
    cd build01
fi

# Find the path where turbojpeg.h is located
# Assuming it is in /usr/local/include/turbojpeg.h
TURBOJPEG="/usr/local/include"

# Install it in /usr/local/encap/virtualgl-v3.1
# Create folder if not exists
if [ ! -d /usr/local/encap/virtualgl-v3.1 ]; then
    mkdir /usr/local/encap/virtualgl-v3.1
    # Change file permissions
    sudo chown -R faiz89:faiz89 /usr/local/encap/virtualgl-v3.1
fi

# Configure
cmake -G"Unix Makefiles"\
    -DCMAKE_INSTALL_PREFIX=/usr/local/encap/virtualgl-v3.1 \
    -DTJPEG_INCLUDE_DIR=$TURBOJPEG \
    -DTJPEG_JAR=/usr/local/share/classes/turbojpeg.jar \
    -DTJPEG_JNILIBRARY=/usr/local/lib/libturbojpeg.so \
    ../../virtualgl

# Set encoding to UTF8 else the make may fail complaining of non-ASCII
# characters
export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8

# Make the file
make -j 4
set +ex

# Finally, install virtualgl here
make install

echo "Now run sudo encap to install it using the encap project."

