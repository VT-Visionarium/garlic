#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash


############################# STUFF TO CONFIGURE ########################

TAR="$topsrcdir/boost_1_60_0.tar.bz2"

URL="https://sourceforge.net/projects/boost/files/boost/1.60.0/boost_1_60_0.tar.bz2/download"

#########################################################################

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

tar -xjf "$TAR" || Fail
cd "$builddir"/"$name" || Fail

./bootstrap.sh --prefix=$prefix || Fail

./b2\
 --prefix=$prefix\
 -d+2\
 -q\
 link=shared\
 variant=debug\
 threading=multi\
 runtime-link=shared\
 install\
 || Fail

PrintSuccess
