#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

# Do not source this more than once.
[ -n "$cscriptdir" ] && return
cscriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$cscriptdir" || exit $?
cscriptdir="$PWD" # now we have full path
[ -z "$topsrcdir" ] && source ../common.bash
unset cscriptdir


# Usage: Install [TAG]
function Install()
{
    GitCreateClone https://github.com/lanceman2/quickscope.git
    GitToBuildDir $1
    # We are now in the newly created build dir
    ./bootstrap --force || Fail
    CFLAGS="-g -Werror -Wall"\
 ./configure\
 --prefix=$prefix\
 --enable-repobuild\
 --enable-debug || Fail

    make -j$ncores || Fail
    make -j3 install || Fail

    PrintSuccess
}
