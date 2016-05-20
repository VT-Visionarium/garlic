#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash

# The source to VMD can only be downloaded interactively
# See ../README

TAR=$topsrcdir/vmd-1.9.2.src.tar.gz

name=vmd-1.9.2 # override from ../../common.bash

if [ ! -f $TAR ] ; then
    echo
    echo "  file $TAR not found"
    echo "  download it interactively from URL:"
    echo
    echo "    http://www.ks.uiuc.edu/Research/vmd/"
    echo
    Fail
fi

#########################################################################

builddir=
# create a new build_03/ or build_04/ or so on.
MkBuildDir builddir
cd "$builddir" || Fail

set -x

tar -xzf "$TAR" || Fail

export PLUGINDIR="$prefix/plugins"

cd "$builddir"/plugins || Fail
# parallel make fails
#make -j$ncores LINUXAMD64 || Fail
make LINUXAMD64 || Fail
make -j3 distrib || Fail

cd "$builddir"/$name || Fail

export VMDINSTALLNAME=$name
export VMDINSTALLBINDIR=$prefix/bin
export VMDINSTALLLIBRARYDIR=$prefix/lib
# We can watch these values brake as the OS changes.
export TCL_INCLUDE_DIR=/usr/include/tcl8.6
export TK_INCLUDE_DIR=$TCL_INCLUDE_DIR

# This is a very strange thing they require
ln -s "$PLUGINDIR" .

# VMD BUG: Looks like build option SHARED makes the name of the vmd exec
# bin file to be vmd.so, which then it not found when the wrapper script
# tries to run it.  I see no easy way to report this bug.  Trying by
# emailing vmd@ks.uiuc.edu.  The web site has "VMD Community Pages" as
# a blocked page which requires a name and password

./configure\
 LINUXAMD64\
 OPENGL\
 GCC\
 TCL\
 TK\
 SHARED\
 PTHREADS\
 || Fail

cd src || Fail
make || Fail
#make -j$ncores || Fail
make -j3 install || Fail

PrintSuccess
