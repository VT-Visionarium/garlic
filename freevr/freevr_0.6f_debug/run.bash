#!/bin/bash

set -x
scriptdir=$(dirname $0) || exit $?
cd $scriptdir || exit $?



# Note that there is a symlink in this directory named .freevrrc which is
# found and parsed by the freeVR library
export LD_LIBRARY_PATH=$scriptdir
./travel

