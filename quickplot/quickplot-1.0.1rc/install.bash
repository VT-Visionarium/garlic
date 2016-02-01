#!/bin/bash

source /usr/local/src/common.bash


############################# STUFF TO CONFIGURE ########################

GITDIR="$topsrcdir"/git

#########################################################################

[ -d "$GITDIR" ] || Fail "$GITDIR does not exist as a directory"

builddir=
MkBuildDir builddir

set -x

cd "$GITDIR" || Fail

# dump the source tree of a given version with a git tag $name
git archive  --format=tar $name | $(cd "$builddir" && tar -xf -) || Fail
cd "$builddir" || Fail

./bootstrap || Fail
CFLAGS="-g -Werror -Wall -Wno-deprecated-declarations"\
 ./configure\
 --prefix=$prefix\
 --enable-developer\
 --enable-debug || Fail

make -j10 || Fail # parallel make, woo ho!
make -j3 install || Fail

PrintSuccess
