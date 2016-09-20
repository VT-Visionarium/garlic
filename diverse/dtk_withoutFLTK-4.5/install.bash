#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash

#name = name of this directory and the git tag name

#########################################################################

# To see diverse package options run:
#cmake "$SRCDIR" -L ; exit
# but make sure the cmake does not shit in the local git repo
# so maybe do it will a copy of the repo source tree and not
# SRCDIR as we defined it above.

#[ "$(fltk-config --version)" != 1.3.0 ] && Fail "fltk-config --version is not 1.3.1"

GitCreateClone https://github.com/lanceman2/diverse.git
GitToBuildDir dtk-4.5

set -x
cmake\
 -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DDTK_BUILD_WITH_FLTK:BOOL=OFF\
 -DDGL_BUILD_WITH_FLTK:BOOL=ON\
 -DDGL_BUILD_WITH_FLTK:BOOL=ON\
 -DDIVERSE_BUILD_WITH_DADS:BOOL=OFF\
 -DDIVERSE_BUILD_WITH_DGL:BOOL=OFF\
 -DDADS_BUILD_WITHOUT_DADS:BOOL=ON\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall" || Fail

make -j8 || Fail # parallel make, woo ho!
make install || Fail

PrintSuccess
