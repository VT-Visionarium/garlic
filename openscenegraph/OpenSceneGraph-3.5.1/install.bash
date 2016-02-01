#!/bin/bash

#########################################################################


source /usr/local/src/common.bash


############################# STUFF TO CONFIGURE ########################

GITDIR="$topsrcdir"/git
# OpenSceneGraph examples zip file
DATANAME=OpenSceneGraph-Data-3.4.0
OSGDATAZIP="$topsrcdir/$DATANAME.zip"

#########################################################################


[ -d "$GITDIR" ] || Fail "$GITDIR does not exist as a directory"
[ -e "$OSGDATAZIP" ] || Fail "$OSGDATAZIP does not exist"

# To see package options run:
#cmake "$GITDIR" -L ; exit

builddir=
# create a new build_03/ or build_04/ or so on.
MkBuildDir builddir

set -x

cd "$GITDIR" || Fail

# dump the source tree of a given version with a git tag $name
git archive  --format=tar $name | $(cd "$builddir" && tar -xf -) || Fail
cd "$builddir"

# The BUILD_OSG_PACKAGES and CMAKE_BUILD_WITH_INSTALL_RPATH setting add
# the gcc -rpath option to the linking so shared libraries can be found
# without setting env DL_LIBRARY_PATH to PREFIX/lib64.  Me thinks that the
# OSG package should do this by default. :( This problem does not happen
# when the linker/loader is configured to link path that includes
# PREFIX/lib64 as in /etc/ld.so.conf and stuff.
cmake\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall"\
 -DBUILD_OSG_PACKAGES:BOOL=ON\
 -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON\
 -DCMAKE_INSTALL_RPATH:STRING="$prefix/lib64"\
 -DBUILD_OSG_EXAMPLES:BOOL=ON || Fail

make -j6 VERBOSE=1 || Fail # parallel make, woo ho!
make -j3 install || Fail

# Add the OpenSceneGraph-Data examples data to PREFIX/share/
mkdir -p "$prefix"/share || Fail
cd "$prefix"/share || Fail
unzip "$OSGDATAZIP" || Fail

PrintSuccess
