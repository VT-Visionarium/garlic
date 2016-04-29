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

    GitCreateClone https://gitlab.kitware.com/paraview/paraview.git

    # NOTE:
    #CMake Error at CMakeLists.txt:49 (message):
    #  ParaView requires an out of source Build.  Please create a separate
    #  binary directory and run CMake there.
    GitToBuildDir $1 --separate-src-build # $1 = git tag
    # We need to get git submodules
    
}

function Install()
{
    # Could be a race condition in parallel make
    make VERBOSE=1 -j$ncores || Fail
    #make VERBOSE=1 || Fail
    make -j$ncores install || Fail
}

