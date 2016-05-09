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
# We tried with -DPARAVIEW_INSTALL_DEVELOPMENT_FILES:BOOL=ON
# but 'make install' is broken with that.

cmake ../src\
 -G"Unix Makefiles"\
 -DCMAKE_CXX_FLAGS:STRING="-Wall"\
 -DCMAKE_INSTALL_PREFIX:PATH="$prefix"\
 -DBUILD_DOCUMENTATION:BOOL=ON\
 -DBUILD_EXAMPLES:BOOL=FALSE\
 -DBUILD_SHARED_LIBS:BOOL=ON\
 -DDART_TESTING_TIMEOUT:STRING=3600\
 -DDELIVER_CONTINUOUS_EMAIL:BOOL=Off\
 -DDOXYGEN_GENERATE_HTMLHELP:BOOL=ON\
 -DICET_BUILD_TESTING:BOOL=ON\
 -DICET_USE_MPI:BOOL=ON\
 -DICET_USE_OPENGL:BOOL=ON\
 -DMEMORYCHECK_SUPPRESSIONS_FILE:FILEPATH=\
 -DMPIEXEC_MAX_NUMPROCS:STRING=48\
 -DMPIEXEC_NUMPROC_FLAG:STRING=-np\
 -DModule_vtkGUISupportQtOpenGL:BOOL=ON\
 -DModule_vtkIOGeoJSON:BOOL=ON\
 -DModule_vtkIOMySQL:BOOL=Off\
 -DModule_vtkPointCloud:BOOL=ON\
 -DModule_vtkWrappingPythonCore:BOOL=ON\
 -DPARAVIEW_AUTOLOAD_PLUGIN_VRPlugin:BOOL=ON\
 -DPARAVIEW_BUILD_PLUGIN_AcceleratedAlgorithms:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_AnalyzeNIfTIIO:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_ArrowGlyph:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_CDIReader:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_CatalystScriptGeneratorPlugin:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_EyeDomeLighting:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_GMVReader:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_GeodesicMeasurement:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_H5PartReader:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_MobileRemoteControl:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_Moments:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_NonOrthogonalSource:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_PacMan:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_PointSprite:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_PythonQtPlugin:BOOL=FALSE\
 -DPARAVIEW_BUILD_PLUGIN_RGBZView:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_SLACTools:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_SciberQuestToolKit:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_SierraPlotTools:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_StreamingParticles:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_SurfaceLIC:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_ThickenLayeredCells:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_UncertaintyRendering:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_VRPlugin:BOOL=ON\
 -DPARAVIEW_BUILD_QT_GUI:BOOL=ON\
 -DPARAVIEW_CLIENT_RENDER_SERVER_TESTS:BOOL=ON\
 -DPARAVIEW_COLLABORATION_TESTING:BOOL=ON\
 -DPARAVIEW_ENABLE_CATALYST:BOOL=ON\
 -DPARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION:BOOL=ON\
 -DPARAVIEW_ENABLE_MATPLOTLIB:BOOL=ON\
 -DPARAVIEW_ENABLE_PYTHON:BOOL=ON\
 -DPARAVIEW_ENABLE_SPYPLOT_MARKERS:BOOL=ON\
 -DPARAVIEW_ENABLE_VTK_MODULES_AS_NEEDED:BOOL=TRUE\
 -DPARAVIEW_ENABLE_WEB:BOOL=ON\
 -DPARAVIEW_INSTALL_DEVELOPMENT_FILES:BOOL=ON\
 -DPARAVIEW_USE_ICE_T:BOOL=ON\
 -DPARAVIEW_USE_MPI:BOOL=ON\
 -DPARAVIEW_USE_VRPN:BOOL=ON\
 -DVTK_MPI_MAX_NUMPROCS:STRING=48\
 -DVTK_RENDERING_BACKEND:STRING=OpenGL\
 || Fail

Install

file=ParaViewTutorialData.tar.gz
url=http://www.paraview.org/Wiki/images/5/5d/$file
tarfile="$topsrcdir/$file"
if [ ! -f "$tarfile" ] ; then
    set -x
    wget $url -O "$tarfile" || Fail
fi

set -x
cd "$prefix" || Fail
tar -xzvf "$tarfile" || Fail
echo "ParaViewTutorialData" > "$prefix/encap.exclude" || Fail

cp $topsrcdir/cave-mono.pvx $prefix || Fail

PrintSuccess

