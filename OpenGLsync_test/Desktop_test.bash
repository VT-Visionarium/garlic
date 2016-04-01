#!/bin/bash -x

scriptdir="$(dirname $0)" || exit 1
cd "$scriptdir" || exit $?

make || exit $?

export CHILD_DISPLAY=:0.0
./sync_test

