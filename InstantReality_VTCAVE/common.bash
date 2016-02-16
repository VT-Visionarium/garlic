#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

[ -z "$topsrcdir" ] && source /usr/local/src/common.bash

GITDIR="$topsrcdir"/git


function _GetSource()
{
    if [ ! -d "$GITDIR" ] ; then
        set -x
        git clone https://github.com/lanceman2/InstantReality_VTCAVE.git\
 "$GITDIR" || Fail
        set +x
    else
        echo -e "\ngit clone "$GITDIR" was found.\n"
    fi
}

function Install()
{
    _GetSource

    builddir=
    MkBuildDir builddir

    set -x

    cd "$GITDIR" || Fail

    # dump the source tree of a given version with a git tag $name
    git archive  --format=tar $name | $(cd "$builddir" && tar -xf -) || Fail
    cd "$builddir" || Fail

    make -j3 PREFIX=$prefix || Fail # parallel make, woo ho!
    make install PREFIX=$prefix || Fail

    PrintSuccess
}
