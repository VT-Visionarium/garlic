#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

[ -z "$topsrcdir" ] && source /usr/local/src/common.bash

function Install()
{
    [ -z "$3" ] && Fail "Usage: ${FUNCNAME[0]} DATANAME TAG CMAKE_OPTIONS"
    local tag
    local dataname
    dataname="$1"
    tag="$2"
    shift 2

    GitCreateClone https://github.com/openscenegraph/osg.git
    GitToBuildDir $tag

    set -x
    cmake "$@" || Fail
    make VERBOSE=1 -j$ncores || Fail
    make -j3 || Fail

    # Add the OpenSceneGraph-Data examples data to PREFIX/share/
    mkdir -p "$prefix"/share || Fail
    cd "$prefix"/share || Fail
    unzip "$topsrcdir/$dataname.zip" || Fail

    PrintSuccess
}

