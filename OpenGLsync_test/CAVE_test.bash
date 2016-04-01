#!/bin/bash -x

scriptdir="$(dirname $0)" || exit 1
cd "$scriptdir" || exit $?

export CHILD_DISPLAY=:0.1
./sync_test

