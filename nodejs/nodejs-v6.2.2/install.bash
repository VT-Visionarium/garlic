#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash
    
GitCreateClone https://github.com/nodejs/node.git
GitToBuildDir v6.2.2 # tag v6.2.2 is not same as $name

# This is impressive; this built on the first try
# with this simple bash script.

# We are now in the newly created build dir
./configure\
 --prefix=$prefix\
 || Fail

make -j$ncores || Fail
make -j3 install || Fail

PrintSuccess
