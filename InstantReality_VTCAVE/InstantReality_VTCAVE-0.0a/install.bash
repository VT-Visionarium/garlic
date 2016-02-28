#!/bin/bash

# this will also source ../common.bash
scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
source ../../common.bash

# Not much changes between this packages different of versions
# installations, so ../common.bash does most of the work.
Install
