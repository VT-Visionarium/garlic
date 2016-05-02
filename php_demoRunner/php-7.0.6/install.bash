#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
# this will source ../common.bash too
source ../../common.bash || exit 1

# get the build dir ready
PreInstall

php_ini=$prefix/share/php.ini

CFLAGS="-g -Wall"\
 CXXFLAGS="-g -Wall"\
 ./configure\
 --prefix="$prefix"\
 --enable-debug\
 --with-config-file-path=$php_ini\
 --disable-all\
 --enable-posix\
 || Fail

make -j$ncores || Fail
make -j3 install || Fail

mkdir -p $prefix/share || Fail
cp $scriptdir/php.ini $php_ini || Fail
echo "php.ini" > $prefix/share/encap.exclude

PrintSuccess
