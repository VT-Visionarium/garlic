#!/bin/bash


source /usr/local/src/common.bash

tag="v7.0.0"
SetupBuildDir "$tag"

cmake\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall"\
 -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON\
 -DCMAKE_INSTALL_RPATH:STRING="$prefix/lib64" || Fail

make -j$ncores || Fail
make -j2 install

