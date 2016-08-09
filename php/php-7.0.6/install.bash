#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
# this will source ../common.bash too
source ../../common.bash || exit 1

# get the build dir ready
PreInstall


CFLAGS="-g -Wall"\
 CXXFLAGS="-g -Wall"\
 ./configure\
 --prefix="$prefix"\
 --enable-debug\
 --disable-all\
 --enable-posix\
 || Fail

make -j$ncores || Fail
make -j3 install || Fail

PrintSuccess
