#!/bin/bash

source /usr/local/src/common.bash


############################# STUFF TO CONFIGURE ########################

TAR="$topsrcdir/$name-source.tar.bz2"

#########################################################################

URL="http://repository.timesys.com/buildsources/f/fltk/fltk-1.1.10/fltk-1.1.10-source.tar.bz2"

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

tar -xvf "$TAR" || Fail
cd "$builddir"/"$name" || Fail

CFLAGS="-g"\
 ./configure\
 --prefix=$prefix\
 --enable-shared || Fail
make -j10 || Fail # parallel make, woo ho!
make install || Fail

PrintSuccess
