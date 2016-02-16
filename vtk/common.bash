#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

[ -z "$topsrcdir" ] && source /usr/local/src/common.bash

function SetupBuildDir()
{
    [ -z "$1" ] && Fail "Usage: ${FUNCNAME[0]} TAG"
    GitCreateClone https://gitlab.kitware.com/vtk/vtk.git git
    GitToBuildDir "$1"
}
