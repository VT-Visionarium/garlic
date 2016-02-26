#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[$i]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
# this will source ../common.bash too
source ../../common.bash

PreInstall $name # git tag

set -x
cmake\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DTJPEG_LIBRARY="-L/usr/lib/x86_64-linux-gnu -lturbojpeg"\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall"

Install
