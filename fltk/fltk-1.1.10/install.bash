#!/bin/bash

source /usr/local/src/common.bash


############################# STUFF TO CONFIGURE ########################

TAR="$topsrcdir/$name-source.tar.gz"

#########################################################################


[ -e "$TAR" ] || Fail "TAR file $TAR does not exist"


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
