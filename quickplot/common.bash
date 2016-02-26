#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

# Do not source this more than once.
[ -n "$cscriptdir" ] && return
cscriptdir="$(dirname ${BASH_SOURCE[$i]})" || exit $?
cd "$cscriptdir" || exit $?
cscriptdir="$PWD" # now we have full path
[ -z "$topsrcdir" ] && source ../common.bash
unset cscriptdir


function Install()
{
    GitCreateClone https://github.com/lanceman2/quickplot.git
    GitToBuildDir
    # We are now in the newly created build dir
    ./bootstrap || Fail
    CFLAGS="-g -Werror -Wall -Wno-deprecated-declarations"\
 ./configure\
 --prefix=$prefix\
 --enable-developer\
 --enable-debug || Fail

    make -j$ncores || Fail
    make -j3 install || Fail

    PrintSuccess
}
