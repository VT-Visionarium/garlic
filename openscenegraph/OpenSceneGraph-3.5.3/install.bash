#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash


ZIP=OpenSceneGraph-Data-3.4.0.zip
ZIPPATH="$topsrcdir/$ZIP"

if [ ! -f "$ZIPPATH" ] ; then
    set -x
    wget\
 http://trac.openscenegraph.org/downloads/developer_releases/$ZIP\
 -O "$ZIPPATH" || Fail
    set +x
fi


GitCreateClone https://github.com/openscenegraph/osg.git
GitToBuildDir # $1 = git tag  default tag = $name

# We got a error from deprecated code at about 99% make completion
# so we found that we had to add compiler flag
# -Wno-deprecated-declarations

set -x
cmake\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall -Werror -Wno-deprecated-declarations"\
 -DBUILD_OSG_PACKAGES:BOOL=ON\
 -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON\
 -DCMAKE_INSTALL_RPATH:STRING="$prefix/lib64"\
 -DBUILD_OSG_EXAMPLES:BOOL=ON || Fail

#make -j seems to be spawning too many processes
#make VERBOSE=1 -j || Fail
make VERBOSE=1 -j$ncores || Fail
make -j3 install || Fail

# Add the OpenSceneGraph-Data examples data to PREFIX/share/
mkdir -p "$prefix"/share || Fail
cd "$prefix"/share || Fail
unzip "$ZIPPATH" || Fail

PrintSuccess
