#!/bin/bash


source /usr/local/src/common.bash

DATANAME=OpenSceneGraph-Data-3.4.0

Install $DATANAME $name\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall"\
 -DBUILD_OSG_PACKAGES:BOOL=ON\
 -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON\
 -DCMAKE_INSTALL_RPATH:STRING="$prefix/lib64"\
 -DBUILD_OSG_EXAMPLES:BOOL=ON

