#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
# this will source ../common.bash too
source ../../common.bash

# There does not appear to be a proper way to install
# VRPN for using the is900 tracker, so this is a mess.

IS_name=ProductCD_IS900_2014a
IS_zip=$topsrcdir/${IS_name}.zip
IS_url="http://www.intersense.com/uploads/archive/${IS_name}.zip"
IS_dir=$topsrcdir/$IS_name

# First install InterSense library
if [ ! -f "$IS_zip" ] ; then
    wget "$IS_url" -O "$IS_zip" || Fail
fi

if [ ! -d "$IS_dir" ] ; then
    cd "$topsrcdir" || Fail
    unzip "$IS_zip" || Fail
    cd - || Fail
fi

mkdir -p "$prefix/lib" "$prefix/include" || Fail
#cp "$IS_dir/SDK/isense.h" "$prefix/include" || Fail
cp "$IS_dir/SDK/Linux/x86_64/libisense.so" "$prefix/lib" || Fail

PreInstall v07.33 # git tag


set -x
cmake ../src\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON\
 -DVRPN_INCLUDE_INTERSENSE:BOOL=ON\
 -DINTERSENSE_INCLUDE_DIR:PATH="$IS_dir/SDK/Linux/Sample"\
 -DINTERSENSE_LIBRARY:FILEPATH="$prefix/lib/libisense.so"\
 || Fail

Install

# VRPN is an installation pig with file name space pollution.
encap_exclude_file="$(dirname $prefix)/encap.exclude"
if ! grep -q "$name" "$encap_exclude_file" ; then
    echo "$name" >> "$encap_exclude_file"
fi
if ! grep -q "$name" "$encap_exclude_file" ; then
    Fail "file $encap_exclude_file does not exist"
fi

PrintSuccess

