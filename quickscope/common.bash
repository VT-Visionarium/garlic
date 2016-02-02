#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

[ -z "$topsrcdir" ] && source /usr/local/src/common.bash

function Install()
{
    GitCreateClone https://github.com/lanceman2/quickscope.git
    GitToBuildDir
    # We are now in the newly created build dir
    ./bootstrap --force || Fail
    CFLAGS="-g -Werror -Wall"\
 ./configure\
 --prefix=$prefix\
 --enable-repobuild\
 --enable-debug || Fail

    make -j$ncores || Fail
    make -j3 install || Fail

    PrintSuccess
}
