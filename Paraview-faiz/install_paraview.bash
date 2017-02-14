#!/bin/bash

# Dependencies
# a) MPI
# b) VRPN
# c) Qt
# d) Python

set -e

attempt=1

scriptdir=$(dirname ${BASH_SOURCE[0]})
cd $scriptdir
scriptdir=$PWD

builddir=$scriptdir/build-v5.2.0-$attempt
PREFIX=/usr/local/encap/paraview-v5.2.0-lance

if [ -e "$builddir" ] ; then
    echo "build dir $builddir exists"
    exit 1
fi

if [ -e "$PREFIX" ] ; then
    echo "PREFIX $PREFIX exists"
    exit 1
fi


# Check if there already is a git repo downloaded in ./git
if [ ! -f "git/ParaView/.git/config" ]; then
    mkdir -p git
    echo "Cloning ParaView from GitHub..."
    # Clone the paraview repo from GitHub
    git clone https://github.com/Kitware/ParaView.git git/ParaView
fi

set -x

cd git/ParaView

# Checkout the version you want
# We want v5.2.0
git checkout v5.2.0

# Update the submodule
git submodule update --init

# Go the the source files and create a build directory in it
mkdir $builddir # this will fail do not change the attempt number
cd $builddir

cmake\
 -DCMAKE_BUILD_TYPE=Release\
 -DBUILD_SHARED_LIBS:BOOL=ON\
 -DCMAKE_INSTALL_PREFIX=$PREFIX\
 -DPARAVIEW_ENABLE_PYTHON:BOOL=ON\
 -DPARAVIEW_AUTOLOAD_PLUGIN_VRPlugin:BOOL=ON\
 -DPARAVIEW_BUILD_PLUGIN_VRPlugin:BOOL=ON\
 -DPARAVIEW_USE_VRPN:BOOL=ON\
 -DPARAVIEW_USE_MPI:BOOL=ON\
 -DPARAVIEW_USE_VRPN:BOOL=ON\
 -DBUILD_TESTING:BOOL=OFF\
 ../git/ParaView

make -j $(nprocs) VERBOSE=1

make install VERBOSE=1

set +x
echo "Almost all done. Run encap to finalize the installation."

