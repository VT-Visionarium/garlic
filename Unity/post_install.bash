#!/bin/bash

scriptdir=$(dirname $0) || exit $?
cd $scriptdir || exit $?

prefix=/usr/local/encap/unity-editor-5.4.0b18+20160524
unity=$prefix/bin/Unity

set -x

mkdir -p $prefix/bin || exit 1
echo "#!/bin/bash" > $unity || exit 1
echo "# This is a generated file" >> $unity || exit 1
cat Unity README >> $unity || exit 1

set +x
ls --color=auto -R $prefix
echo -e "\n\nSUCCESS"
echo -e "\nyou may want to run:\n"
echo -e "  sudo encap\n"
