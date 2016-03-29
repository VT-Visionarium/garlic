#!/bin/bash

scriptdir="$(dirname ${BASH_SOURCE[0]})" || exit $?
cd "$scriptdir" || exit $?
scriptdir="$PWD" # now we have full path
# this will source ../common.bash too
source ../../common.bash

# we make the install prefix name different from the git tag because the
# paraview tags do not have the string "paraview" them
PreInstall "v5.0.1" # git tag
set -x
cd ../src || Fail
GitTarSubMod VTK "v7.0.0"
GitTarSubMod Utilities/VisItBridge HEAD
GitTarSubMod ThirdParty/protobuf/vtkprotobuf HEAD
GitTarSubMod ThirdParty/IceT/vtkicet HEAD
GitTarSubMod ThirdParty/QtTesting/vtkqttesting HEAD
cd ../build || Fail

set -x
cmake ../src\
 -G"Unix Makefiles"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DCMAKE_CXX_FLAGS:STRING="-g -Wall" || Fail

Install

