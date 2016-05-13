#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash

GitCreateClone git://git.code.sf.net/p/opensg/code
GitToBuildDir 741444a7636e --separate-src-build

set -x
cmake ../src\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 || Fail


make VERBOSE=1 -j$ncores || Fail
make VERBOSE=1 -j3 install || Fail

PrintSuccess
