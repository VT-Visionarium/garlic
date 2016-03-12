#!/bin/bash

# Not much changes between this packages different of versions
# installations, so ../common.bash does most of the work.

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
# this will source ../common.bash too
source ../../common.bash


PreInstall $name # git tag

# note:  Could not find a apt-get package for the TurboJPEG JAR file
# /opt/libjpeg-turbo/classes/turbojpeg.jar.  TODO: add it later.
# for now we have -DTVNC_BUILDJAVA=0

set -x
cmake .\
 -G "Unix Makefiles"\
 -DTJPEG_LIBRARY="-L/usr/lib/x86_64-linux-gnu -lturbojpeg"\
 -DTVNC_BUILDJAVA=0\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 || Fail

Install
