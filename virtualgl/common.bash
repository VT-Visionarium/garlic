#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

[ -z "$topsrcdir" ] && source /usr/local/src/common.bash

# Usage: Install TAG CMAKE_OPTIONS
function Install()
{
    [ -z "$2" ] && Fail "Usage: ${FUNCNAME[0]} TAG CMAKE_OPTIONS"
    local tag
    tag="$1"
    shift 1

    GitCreateClone https://github.com/VirtualGL/virtualgl.git
    GitToBuildDir $tag

    set -x
    cmake "$@" || Fail
    make VERBOSE=1 -j$ncores || Fail
    make -j3 || Fail

    PrintSuccess
}

