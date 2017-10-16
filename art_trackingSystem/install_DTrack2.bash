#!/bin/bash

set -ex

gui_name=DTrack2_v2.13.0
cli_name=DTrack2CLI_v1.0.0
prefix=/usr/local/encap/$gui_name


cd "$(dirname BASH_SOURCE[0])"
[ -d $gui_name ] || tar -xzf ${gui_name}_linux64.tar.gz
[ -d $cli_name ] || tar -xzf ${cli_name}_linux64.tar.gz

rm -rf $prefix
mkdir -p $prefix/bin $prefix/libs/
cp libpng12.so.0.54.0 $prefix/libs/
ln -s libpng12.so.0.54.0 $prefix/libs/libpng12.so.0
cp -P $gui_name/libs/* $prefix/libs/
cp -r $gui_name/bin/* $prefix/bin/
cp $cli_name/DTrack2CLI $prefix/bin/
cp encap.exclude $prefix/
cp encap.exclude.bin $prefix/bin/encap.exclude
cp DTrack2 $prefix/bin
chmod 755 $prefix/bin/DTrack2
set +x
echo -e "\nSUCCESSFULLY installed $gui_name and $cli_name in $prefix\n"
