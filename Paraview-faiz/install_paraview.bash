#!/bin/bash

# Dependencies
# a) MPI
# b) VRPN
# c) Qt
# d) Python

# Go to the location from where you want to run this script
cd ~/git

# Check if there already is a git repo downloaded in ~/git
PARAVIEW="$(ls /home/faiz89/git | grep -w ParaView)"
if [ -z "$PARAVIEW" ]; then
	set -ex
	echo "Cloning ParaView from GitHub..."
	# Clone the paraview repo from GitHub
	git clone https://github.com/Kitware/ParaView.git
	set +ex
fi

# Checkout the version you want
# We want v5.2.0
git checkout v5.2.0

# Update the submodule
git submodule update --init

# Go the the source files and create a build directory in it
cd ParaView && mkdir build-v5.2.0 && cd build-v5.2.0

set -ex
if cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS:BOOL=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local/encap/paraview-v5.2.0 \
    -DPARAVIEW_AUTOLOAD_PLUGIN_VRPlugin:BOOL=ON \
    -DPARAVIEW_BUILD_PLUGIN_VRPlugin:BOOL=ON \
    -DPARAVIEW_ENABLE_PYTHON:BOOL=ON \
    -DPARAVIEW_USE_MPI:BOOL=ON \
    -DPARAVIEW_USE_VRPN:BOOL=ON \
	-DBUILD_TESTING:BOOL=OFF \
    ../../ParaView; then
echo "ParaView compiled successfully."
else
    "ParaView compilation unsuccessful."
fi

make -j 8

make install

echo "Almost all done. Run encap to finalize the installation."

