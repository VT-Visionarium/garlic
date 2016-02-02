#!/bin/bash

source /usr/local/src/common.bash


Install 2.4.1\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DTJPEG_LIBRARY="-L/usr/lib/x86_64-linux-gnu -lturbojpeg"\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall"

