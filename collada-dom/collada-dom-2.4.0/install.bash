#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash


############################# STUFF TO CONFIGURE ########################

TAR="$topsrcdir/$name.tgz"

#########################################################################

# We imagine that this URL will break in the near future.
URL="https://sourceforge.net/projects/collada-dom/files/Collada%20DOM/Collada%20DOM%202.4/collada-dom-2.4.0.tgz/download"

if [ ! -f "$TAR" ] ; then
    set -x
    wget $URL -O $TAR || Fail
fi


#########################################################################

builddir=
# create a new build_03/ or build_04/ or so on.
MkBuildDir builddir
cd "$builddir" || Fail

set -x

tar -xzf "$TAR" || Fail
mkdir "$builddir"/"$name"/build || Fail
cd "$builddir"/"$name"/build || Fail

cmake ..\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall"\
 || Fail

make VERBOSE=1 -j$ncores || Fail
make VERBOSE=1 -j3 install || Fail

PrintSuccess
