#!/bin/bash

# This is sourced by install.bash file in the directories in this
# directory.

[ -z "$topsrcdir" ] && source /usr/local/src/common.bash

function Install()
{
    GitCreateClone https://github.com/lanceman2/quickplot.git
    GitToBuildDir
    # We are now in the newly created build dir
    ./bootstrap || Fail
    CFLAGS="-g -Werror -Wall -Wno-deprecated-declarations"\
 ./configure\
 --prefix=$prefix\
 --enable-developer\
 --enable-debug || Fail

    make -j$ncores || Fail
    make -j3 install || Fail

    PrintSuccess
}
