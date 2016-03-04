#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash


############################# STUFF TO CONFIGURE ########################

TAR="$topsrcdir/$name.tar.gz"


#########################################################################

URL="http://idav.ucdavis.edu/~okreylos/ResDev/Vrui/$name.tar.gz"

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

tar -xf "$TAR" || Fail
cd "$builddir"/"$name" || Fail

make INSTALLDIR="$prefix"  -j || Fail # parallel make, woo ho!
make INSTALLDIR="$prefix" install || Fail

#cd "$builddir"/"$name"/ExamplePrograms && make INSTALLDIR="$prefix" || Fail

PrintSuccess #"run:\n$buildir/$name/ExamplePrograms/bin/ShowEarthModels\n"
