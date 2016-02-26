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
    GitCreateClone clone https://github.com/lanceman2/InstantReality_VTCAVE.git
    GitToBuildDir

    make -j3 PREFIX=$prefix || Fail # parallel make, woo ho!
    make install PREFIX=$prefix || Fail

    PrintSuccess
}
