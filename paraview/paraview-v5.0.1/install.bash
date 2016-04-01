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
 -DBUILD_EXAMPLES:BOOL=ON\
 -DBUILD_SHARED_LIBS:BOOL=ON\
 -DBUILD_TESTING:BOOL=ON\
 -DCMAKE_BUILD_TYPE:STRING=Debug\
 -DCMAKE_COLOR_MAKEFILE:BOOL=ON\
 -DCMAKE_CXX_FLAGS_DEBUG:STRING=-g\
 -DCMAKE_VERBOSE_MAKEFILE:BOOL=TRUE\
 -DICET_BUILD_TESTING:BOOL=ON\
 -DICET_USE_MPI:BOOL=ON\
 -DICET_USE_OPENGL:BOOL=ON\
 -DModule_AutobahnPython:BOOL=OFF\
 -DModule_PoissonReconstruction:BOOL=OFF\
 -DModule_Pygments:BOOL=OFF\
 -DModule_SixPython:BOOL=OFF\
 -DModule_Twisted:BOOL=OFF\
 -DModule_VisItLib:BOOL=OFF\
 -DModule_ZopeInterface:BOOL=OFF\
 -DModule_pqDeprecated:BOOL=OFF\
 -DModule_pqPython:BOOL=OFF\
 -DModule_vtkAcceleratorsDax:BOOL=OFF\
 -DModule_vtkAcceleratorsPiston:BOOL=OFF\
 -DModule_vtkAddon:BOOL=OFF\
 -DDOXYGEN_GENERATE_HTMLHELP:BOOL=ON\
 -DModule_vtkCosmoHaloFinder:BOOL=OFF\
 -DModule_vtkDICOM:BOOL=OFF\
 -DModule_vtkDomainsChemistryOpenGL2:BOOL=OFF\
 -DModule_vtkFiltersMatlab:BOOL=OFF\
 -DModule_vtkFiltersReebGraph:BOOL=OFF\
 -DModule_vtkFiltersSMP:BOOL=OFF\
 -DModule_vtkFiltersSelection:BOOL=OFF\
 -DModule_vtkFiltersStatisticsGnuR:BOOL=OFF\
 -DModule_vtkGUISupportQtOpenGL:BOOL=OFF\
 -DModule_vtkGUISupportQtSQL:BOOL=OFF\
 -DModule_vtkGUISupportQtWebkit:BOOL=OFF\
 -DModule_vtkGeovisCore:BOOL=OFF\
 -DModule_vtkIOADIOS:BOOL=OFF\
 -DModule_vtkIOFFMPEG:BOOL=OFF\
 -DModule_vtkIOGDAL:BOOL=OFF\
 -DModule_vtkIOGeoJSON:BOOL=OFF\
 -DModule_vtkIOMINC:BOOL=OFF\
 -DModule_vtkIOMySQL:BOOL=OFF\
 -DModule_vtkIOODBC:BOOL=OFF\
 -DModule_vtkIOPostgreSQL:BOOL=OFF\
 -DModule_vtkIOSQL:BOOL=OFF\
 -DModule_vtkIOVideo:BOOL=OFF\
 -DModule_vtkIOVisItBridge:BOOL=OFF\
 -DModule_vtkIOXdmf3:BOOL=OFF\
 -DModule_vtkImagingMath:BOOL=OFF\
 -DModule_vtkImagingStatistics:BOOL=OFF\
 -DModule_vtkImagingStencil:BOOL=OFF\
 -DModule_vtkInfovisBoost:BOOL=OFF\
 -DModule_vtkInfovisBoostGraphAlgorithms:BOOL=OFF\
 -DModule_vtkInfovisLayout:BOOL=OFF\
 -DModule_vtkInfovisParallel:BOOL=OFF\
 -DModule_vtkPVVTKExtensionsCGNSReader:BOOL=OFF\
 -DModule_vtkPVVTKExtensionsCosmoTools:BOOL=OFF\
 -DModule_vtkParaViewWebDocumentation:BOOL=OFF\
 -DModule_vtkPython:BOOL=OFF\
 -DModule_vtkPythonInterpreter:BOOL=OFF\
 -DModule_vtkRenderingContextOpenGL2:BOOL=OFF\
 -DModule_vtkRenderingExternal:BOOL=OFF\
 -DModule_vtkRenderingFreeTypeFontConfig:BOOL=OFF\
 -DModule_vtkRenderingImage:BOOL=OFF\
 -DModule_vtkRenderingLICOpenGL2:BOOL=OFF\
 -DModule_vtkRenderingMatplotlib:BOOL=OFF\
 -DModule_vtkRenderingOpenGL2:BOOL=OFF\
 -DModule_vtkRenderingQt:BOOL=OFF\
 -DModule_vtkRenderingTk:BOOL=OFF\
 -DModule_vtkRenderingVolumeOpenGL2:BOOL=OFF\
 -DModule_vtkTclTk:BOOL=OFF\
 -DModule_vtkTestingIOSQL:BOOL=OFF\
 -DModule_vtkUtilitiesBenchmarks:BOOL=OFF\
 -DModule_vtkUtilitiesColorSeriesToXML:BOOL=OFF\
 -DModule_vtkViewsGeovis:BOOL=OFF\
 -DModule_vtkViewsInfovis:BOOL=OFF\
 -DModule_vtkViewsQt:BOOL=OFF\
 -DModule_vtkWebApplications:BOOL=OFF\
 -DModule_vtkWebCore:BOOL=OFF\
 -DModule_vtkWebInstall:BOOL=OFF\
 -DModule_vtkWebJavaScript:BOOL=OFF\
 -DModule_vtkWebPython:BOOL=OFF\
 -DModule_vtkWrappingJava:BOOL=OFF\
 -DModule_vtkWrappingPythonCore:BOOL=OFF\
 -DModule_vtkWrappingTcl:BOOL=OFF\
 -DModule_vtkglew:BOOL=OFF\
 -DModule_vtklibproj4:BOOL=OFF\
 -DModule_vtkmpi4py:BOOL=OFF\
 -DModule_vtksqlite:BOOL=OFF\
 -DModule_vtkxdmf3:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_AcceleratedAlgorithms:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_AnalyzeNIfTIIO:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_ArrowGlyph:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_CDIReader:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_EyeDomeLighting:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_GMVReader:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_GeodesicMeasurement:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_H5PartReader:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_MobileRemoteControl:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_Moments:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_NonOrthogonalSource:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_PacMan:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_PointSprite:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_RGBZView:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_SLACTools:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_SciberQuestToolKit:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_SierraPlotTools:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_StreamingParticles:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_SurfaceLIC:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_ThickenLayeredCells:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_UncertaintyRendering:BOOL=OFF\
 -DPARAVIEW_AUTOLOAD_PLUGIN_VRPlugin:BOOL=ON\
 -DPARAVIEW_BUILD_CATALYST_ADAPTORS:BOOL=OFF\
 -DPARAVIEW_BUILD_PLUGIN_AcceleratedAlgorithms:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_AdiosReader:BOOL=FALSE\
 -DPARAVIEW_BUILD_PLUGIN_AnalyzeNIfTIIO:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_ArrowGlyph:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_CDIReader:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_EyeDomeLighting:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_ForceTime:BOOL=FALSE\
 -DPARAVIEW_BUILD_PLUGIN_GMVReader:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_GeodesicMeasurement:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_H5PartReader:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_InSituExodus:BOOL=FALSE\
 -DPARAVIEW_BUILD_PLUGIN_MantaView:BOOL=FALSE\
 -DPARAVIEW_BUILD_PLUGIN_MobileRemoteControl:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_Moments:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_NonOrthogonalSource:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_PacMan:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_PointSprite:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_RGBZView:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_SLACTools:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_SciberQuestToolKit:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_SierraPlotTools:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_StreamingParticles:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_SurfaceLIC:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_ThickenLayeredCells:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_UncertaintyRendering:BOOL=TRUE\
 -DPARAVIEW_BUILD_PLUGIN_VRPlugin:BOOL=ON\
 -DPARAVIEW_BUILD_PLUGIN_VaporPlugin:BOOL=FALSE\
 -DPARAVIEW_BUILD_QT_GUI:BOOL=ON\
 -DPARAVIEW_BUILD_WEB_DOCUMENTATION:BOOL=ON\
 -DPARAVIEW_CLIENT_RENDER_SERVER_TESTS:BOOL=ON\
 -DPARAVIEW_COLLABORATION_TESTING:BOOL=ON\
 -DPARAVIEW_DATA_EXCLUDE_FROM_ALL:BOOL=OFF\
 -DPARAVIEW_ENABLE_CATALYST:BOOL=ON\
 -DPARAVIEW_ENABLE_CGNS:BOOL=OFF\
 -DPARAVIEW_ENABLE_COSMOTOOLS:BOOL=OFF\
 -DPARAVIEW_ENABLE_FFMPEG:BOOL=OFF\
 -DPARAVIEW_ENABLE_PYTHON:BOOL=OFF\
 -DPARAVIEW_ENABLE_SPYPLOT_MARKERS:BOOL=ON\
 -DPARAVIEW_ENABLE_VTK_MODULES_AS_NEEDED:BOOL=TRUE\
 -DPARAVIEW_ENABLE_XDMF3:BOOL=OFF\
 -DPARAVIEW_EXTERNAL_PLUGIN_DIRS:STRING=\
 -DPARAVIEW_EXTRA_EXTERNAL_PLUGINS:STRING=\
 -DPARAVIEW_INITIALIZE_MPI_ON_CLIENT:BOOL=OFF\
 -DPARAVIEW_INSTALL_DEVELOPMENT_FILES:BOOL=OFF\
 -DPARAVIEW_PLUGIN_LOADER_PATHS:STRING=\
 -DPARAVIEW_USE_ATP:BOOL=OFF\
 -DPARAVIEW_USE_DAX:BOOL=OFF\
 -DPARAVIEW_USE_ICE_T:BOOL=ON\
 -DPARAVIEW_USE_MPI:BOOL=ON\
 -DICET_USE_MPI:BOOL=ON\
 -DICET_USE_OPENGL:BOOL=ON\
 -DPARAVIEW_USE_MPI_SSEND:BOOL=OFF\
 -DPARAVIEW_USE_PISTON:BOOL=OFF\
 -DPARAVIEW_USE_QTWEBKIT:BOOL=OFF\
 -DPARAVIEW_USE_VISITBRIDGE:BOOL=OFF\
 -DPARAVIEW_USE_VRPN:BOOL=ON\
 -DPARAVIEW_USE_VRUI:BOOL=OFF\
 -DPARAVIEW_ENABLE_SPYPLOT_MARKERS:BOOL=ON\
 -DPARAVIEW_ENABLE_VTK_MODULES_AS_NEEDED:BOOL=TRUE\
 -DPARAVIEW_ENABLE_XDMF3:BOOL=OFF\
 -DPARAVIEW_BUILD_PLUGIN_VRPlugin:BOOL=ON\
 -DPARAVIEW_BUILD_PLUGIN_VaporPlugin:BOOL=FALSE\
 -DPARAVIEW_BUILD_QT_GUI:BOOL=ON\
 -DPARAVIEW_USE_VRPN:BOOL=ON\
 -DPARAVIEW_USE_VRUI:BOOL=OFF\
 -DPV_TEST_USE_RANDOM_PORTS:BOOL=ON\
 -DVTK_Group_Imaging:BOOL=OFF\
 -DVTK_Group_MPI:BOOL=OFF\
 -DVTK_Group_ParaViewCore:BOOL=ON\
 -DVTK_Group_ParaViewQt:BOOL=OFF\
 -DVTK_Group_ParaViewRendering:BOOL=ON\
 -DVTK_Group_Qt:BOOL=OFF\
 -DVTK_Group_Rendering:BOOL=OFF\
 -DVTK_Group_StandAlone:BOOL=OFF\
 -DVTK_Group_Tk:BOOL=OFF\
 -DVTK_Group_Views:BOOL=OFF\
 -DVTK_Group_Web:BOOL=OFF\
 -DVTK_IGNORE_GLDRIVER_BUGS:BOOL=OFF\
 -DVTK_IOS_BUILD:BOOL=OFF\
 -DVTK_LEGACY_REMOVE:BOOL=OFF\
 || Fail

Install


file=ParaViewTutorialData.tar.gz
url=http://www.paraview.org/Wiki/images/5/5d/$file
tarfile="$srcdir/$file"
if [ ! -f "$tarfile" ] ; then
    set -x
    wget $url -O "$tarfile" || Fail
fi

set -x
cd "$prefix" || Fail
tar -xzvf "$tarfile" || Fail
echo "ParaViewTutorialData" > "$prefix/encap.exclude" || Fail
