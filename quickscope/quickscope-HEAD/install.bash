#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
# this will source ../common.bash too
source ../../common.bash

# Not much changes between this packages different of versions
# installations, so ../common.bash does most of the work.

# We build from the source repo checkout with tag HEAD
Install HEAD
