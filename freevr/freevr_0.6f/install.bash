#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash

# The source to VMD can only be downloaded interactively
# See ../README

TAR=$topsrcdir/freevr_0.6f.tar.gz
URL="http://www.freevr.org/Downloads/freevr_0.6f.tar.gz"

T_TAR=$topsrcdir/fvtutogl_vr2011.tar.gz
T_URL="http://freevr.org/Downloads/fvtutogl_vr2011.tar.gz"

if [ ! -f $TAR ] ; then
    set -x
    wget $URL -O $TAR || Fail
    [ -f $TAR ] || Fail
fi

if [ ! -f $T_TAR ] ; then
    set -x
    wget $T_URL -O $T_TAR || Fail
    [ -f $T_TAR ] || Fail
fi




######################################################################

builddir=
# create a new build_03/ or build_04/ or so on.
MkBuildDir builddir
cd "$builddir" || Fail

set -x

tar -xzf "$TAR" || Fail

cd $name/src || Fail

tab="$(echo -en '\t')"

cat << EOF >> Makefile || Fail
# added by lance
install:
${tab}mkdir -p $prefix/lib $prefix/include $prefix/bin $prefix/etc
${tab}cp libfr*.a freevr.h vr_*.h $prefix/lib
${tab}cp freevr.h vr_*.h $prefix/include
${tab}cp travel $prefix/bin/freevr_test
${tab}cp $topsrcdir/hy_VT_freevrrc $topsrcdir/rc_viscube_c4t2_dualhead_all_4procs  $prefix/etc

EOF

make -j$ncores linux2.6-glx-64 || Fail
make install || Fail


tutdir=$prefix/tutorial
mkdir -p $tutdir || Fail
cd $tutdir || Fail
tar -xzf $T_TAR || Fail

cd $tutdir/FVtut_ogl/Code || Fail
# patch file made with:
# diff originalfile updatedfile > file.patch
mv Makefile Makefile.org || Fail
patch Makefile.org\
 -i $scriptdir/tutorial_Makefile.patch\
 -o Makefile || Fail

echo "tutorial" > $prefix/encap.exclude

PrintSuccess
