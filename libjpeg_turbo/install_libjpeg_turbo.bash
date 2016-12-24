#!/bin/bash

# To install TurboVNC, we need to install some dependencies first
# The first one is libjpeg-turbo SDK - https://github.com/libjpeg-turbo/libjpeg-turbo

# Go to the location from where you want to run this script
cd /home/faiz89/git

# libjpeg-turbo SDK itself has some dependencies
# Make sure you have autoconf 2.56 or later, automake 1.7 or later, nasm or yasm, JDK or OpenJDK"

# Check if autoconf is installed
AUTOCONF="$(whereis autoconf)"
if [ -z "$AUTOCONF" ]; then
    set -ex # Exit error and echo the command
    echo "Installing autoconf..."
    sudo yum install autoconf -y
    set +ex # Unset the exit error and the echo command
else
    echo "autoconf already installed. Not re-installing...."
fi

# Check if automake is installed
AUTOMAKE="$(whereis automake)"
if [ -z "$AUTOMAKE" ]; then
    set -ex
    echo "Installing automake..."
    sudo yum install automake -y
    set +ex
else
    echo "automake already installed. Not re-installing..."
fi

# Check if yasm or nasm installed
NASM="$(whereis nasm)"
YASM="$(whereis yasm)"
if [ -z "$NASM" ] && [ -z "$YASM" ]; then
    set -ex
    echo "Installing yasm...."
    sudo yum install yasm -y
    set +ex 
else
    echo "yasm or nasm or both already installed. Not re-installing..."
fi

# Check if JDK or openjdk is installed
JDK="$(java -version 2>&1 | grep jdk)"
if [ -z "$JDK" ]; then
    set -ex
    echo "Installing openjdk version 1.8.0...."
    sudo yum install java-1.8.0-openjdk -y
    sudo yum install java-1.8.0-openjdk-devel -y
    set +ex
else
    echo "JDK already installed. Not re-installing...."
fi

# Check if there already is a git repo downloaded in ~/git
LIBJPEG="$(ls /home/faiz89/git | grep libjpeg-turbo)"
if [ -z "$LIBJPEG" ]; then
    set -ex
    echo "Cloning libjpeg-turbo from GitHub....."
    # Clone the libjpeg-turbo SDK repo from GitHub
    git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git
    set +ex
else
    echo "There is already a GitHub repo for libjpeg-turbo downloaded in
    /home/faiz89/git. Not re-downloading..."
fi

set -ex
# Building libjpeg-turbo
# Go to the source files
cd libjpeg-turbo
autoreconf -fiv
# Make a build directory
mkdir build01 && cd build01
set +ex
# Find the path of JNI_CFLAGS
JNI_H="$(sudo find /usr/lib -name "jni.h")"
JNI_DIR=$(dirname "${JNI_H}")
# Install it in /usr/local/encap/libjpeg-turbo-v1.5.2
# Create folder if not exists
if [ ! -d /usr/local/encap/libjpeg-turbo-v1.5.2 ]; then
    set -ex
    sudo mkdir /usr/local/encap/libjpeg-turbo-v1.5.2
    # Change file permissions
    sudo chown -R faiz89:faiz89 /usr/local/encap/libjpeg-turbo-v1.5.2 
fi
sh ../../libjpeg-turbo/configure JNI_CFLAGS="-I$JNI_DIR -I$JNI_DIR/linux"\
    --with-pic --with-java --prefix=/usr/local/encap/libjpeg-turbo-v1.5.2
# Set encoding to UTF8 else the make may fail complaining of non-ASCII
# characters
export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8
# Make the file
make -j 4
set +ex

# Finally, install libjpeg-turbo here
make install

