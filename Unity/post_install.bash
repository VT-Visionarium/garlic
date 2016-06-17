#!/bin/bash

scriptdir=$(dirname $0) || exit $?
cd $scriptdir || exit $?

prefix=/usr/local/encap/unity-editor-5.4.0b18+20160524

set -x

mkdir -p $prefix/bin || exit 1
cp Unity $prefix/bin || exit 1

set +x
ls --color=auto -R $prefix
echo -e "\n\nSUCCESS"
echo -e "\nyou may want to run:\n"
echo -e "  sudo encap\n"
