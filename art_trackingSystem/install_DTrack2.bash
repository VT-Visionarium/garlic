#!/bin/bash

set -ex

name=DTrack2_v2.13.0
tarname=${name}_linux64
prefix=/usr/local/encap/$name


cd "$(dirname BASH_SOURCE[0])"
[ -d $name ] || tar -xzf $tarname.tar.gz

rm -rf $prefix
mkdir -p $prefix/bin $prefix/libs
cp libpng12.so.0.54.0 $prefix/libs/
ln -s libpng12.so.0.54.0 $prefix/libs/libpng12.so.0
cp -P $name/libs/* $prefix/libs
cp -r $name/bin/* $prefix/bin/
cp encap.exclude $prefix/
cp encap.exclude.bin $prefix/bin/encap.exclude
cp DTrack2 $prefix/bin
chmod 755 $prefix/bin/DTrack2
set +x
echo -e "\nSUCCESSFULLY installed $name in $prefix\n"
