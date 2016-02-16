#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

[ -z "$topsrcdir" ] && source /usr/local/src/common.bash

function Install()
{
    [ -z "$1" ] && Fail "Usage: ${FUNCNAME[0]} TAG"
    local tag
    tag="$1"
    shift 1

    GitCreateClone https://github.com/TurboVNC/turbovnc.git
    GitToBuildDir $tag

    set -x

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
