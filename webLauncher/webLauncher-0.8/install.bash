#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash
    
GitCreateClone https://github.com/VT-Visionarium/webLauncher.git
GitToBuildDir
#GitToBuildDir tag

# We are now in the newly created build dir
./configure\
 --prefix=$prefix\
 || Fail

make || Fail
make install || Fail

echo "node_modules" > $prefix/bin/encap.exclude
echo "etc" > $prefix/encap.exclude

PrintSuccess
