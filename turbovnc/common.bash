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


function PreInstall()
{
    [ -z "$1" ] && Fail "Usage: ${FUNCNAME[0]} GIT_TAG"

    GitCreateClone https://github.com/TurboVNC/turbovnc.git
    GitToBuildDir $1 # $1 = git tag
}

function Install()
{
    make VERBOSE=1 -j$nprocs || Fail # parallel make, woo ho!
    make -j3 install || Fail

    PrintSuccess
}
