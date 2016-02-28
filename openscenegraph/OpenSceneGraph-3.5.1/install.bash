#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
# this will source ../common.bash too
source ../../common.bash

DATANAME=OpenSceneGraph-Data-3.4.0

PreInstall $name # git tag

set -x
cmake\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall"\
 -DBUILD_OSG_PACKAGES:BOOL=ON\
 -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON\
 -DCMAKE_INSTALL_RPATH:STRING="$prefix/lib64"\
 -DBUILD_OSG_EXAMPLES:BOOL=ON

Install $DATANAME

