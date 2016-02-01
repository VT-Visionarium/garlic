#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

[ -z "$topsrcdir" ] && source /usr/local/src/common.bash

GITDIR="$topsrcdir"/git


function _GetSource()
{
    if [ ! -d "$GITDIR" ] ; then
        set -x
        git clone https://github.com/TurboVNC/turbovnc.git\
 "$GITDIR" || Fail
        set +x
    else
        echo -e "\ngit clone "$GITDIR" was found.\n"
    fi
}

# example: Install 2.0.1
# Usage: Install TAG
function Install()
{
    [ -n "$1" ] || Fail "Usage: Install TAG"
    local tag
    tag="$1"

    _GetSource

    builddir=
    MkBuildDir builddir

    set -x

    cd "$GITDIR" || Fail


    # dump the source tree of a given version with git tag $tag
    git archive  --format=tar "$tag" | $(cd "$builddir" && tar -xf -) || Fail
    cd "$builddir" || Fail

# note:  Could not find a apt-get package for the TurboJPEG JAR file
# /opt/libjpeg-turbo/classes/turbojpeg.jar.  TODO: add it later.
# for now we have -DTVNC_BUILDJAVA=0
    cmake .\
 -G "Unix Makefiles"\
 -DTJPEG_LIBRARY="-L/usr/lib/x86_64-linux-gnu -lturbojpeg"\
 -DTVNC_BUILDJAVA=0\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 || Fail

    make VERBOSE=1 -j$nprocs || Fail # parallel make, woo ho!
    make -j3 install || Fail

    PrintSuccess
}
