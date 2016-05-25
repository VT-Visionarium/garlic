#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash

# The source to VMD can only be downloaded interactively
# See ../README

TAR=$topsrcdir/vmd-1.9.2.src.tar.gz
tutortarfile=$topsrcdir/vmd-tutorial-files.tar.gz
tutorURL="http://www.ks.uiuc.edu/Training/Tutorials/vmd/vmd-tutorial-files.tar.gz"
#name=vmd-1.9.2 # from ../../common.bash

if [ ! -f $TAR ] ; then
    echo
    echo "  file $TAR not found"
    echo "  download it interactively from URL:"
    echo
    echo "    http://www.ks.uiuc.edu/Research/vmd/"
    echo
    Fail
fi

if [ ! -f $tutortarfile ] ; then
    set -x
    wget $tutorURL -O $tutortarfile || Fail
    [ -f $tutortarfile ] || Fail
fi

######################################################################

builddir=
# create a new build_03/ or build_04/ or so on.
MkBuildDir builddir
cd "$builddir" || Fail

set -x

tar -xzf "$TAR" || Fail

export PLUGINDIR="$prefix/plugins"
export TCLINC=-I/usr/include/tcl8.6
export TCLLIB=/usr/lib/x86_64-linux-gnu/libtcl.so
export TKINC=$TCLINC
export TKLIB=/usr/lib/x86_64-linux-gnu/libtk.so

cd "$builddir"/plugins || Fail

# We needed to debug this build so silent needed
# to be removed.
for m in Makefile */Makefile ; do
    sed -e\
 's/^\.SILENT\:.*/# lance removed .SILENT here/g'\
 $m > $m.newZ || Fail
    mv $m.newZ $m || Fail
done

# parallel make fails
#make -j$ncores LINUXAMD64 || Fail
make LINUXAMD64 || Fail
make distrib || Fail


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
# a blocked page which requires a name and password.

# graphic/windowing library:
# I'd guess you'd pick some of these and some combin
# OPENGL
# OPENGLPBUFFER
# SDL
# FLTKOPENGL
# MESA
# CAVE # CAVE libs from VRCO
# FREEVR

./configure\
 LINUXAMD64\
 OPENGL\
 GCC\
 TCL\
 FLTK\
 TK\
 FREEVR\
 PTHREADS\
 || Fail

cd src || Fail
# parallel make fails
#make -j$ncores || Fail
make || Fail
make -j3 install || Fail

#ANOTHER BUG FIX: remove broken rlwrap options from wrapper script
# http://www.ks.uiuc.edu/Research/vmd/mailing_list/vmd-l/19158.html
# Given how old the bug is I'd say that they don't give a shit.
sed -e 's/\-b(){}\[\],&\^\%#\;|\\\\//g'\
 $prefix/bin/$name > $prefix/bin/vmd || Fail
chmod 755 $prefix/bin/vmd || Fail
cp $prefix/bin/vmd $prefix/bin/$name || Fail

set +x
tprefix="$prefix/tutorial"
echo "Installing tutorial files in $tprefix"
set -x
mkdir -p "$tprefix" || Fail
cd "$tprefix" || Fail
tar -xzf "$tutortarfile" || Fail

PrintSuccess
