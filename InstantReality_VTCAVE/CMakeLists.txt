# TODO: rewrite this in GNU make, this is a mess.
# Comment out this next line to use cmake.
message(SEND_ERROR "CMake Blows chunks, try running GNU make by running 'make'")
cmake_minimum_required(VERSION 2.6)

# Set default prefix.  You may want to change the version number suffix

set(CMAKE_INSTALL_PREFIX /usr/local/encap/instantReality_VTCAVE-0.0
    CACHE PATH "Installation Prefix Path")

project(InstantIOPlugin_VTCAVE)

set(DTK_ROOT "/usr/local" CACHE PATH "Path to Diverse ToolKit (DTK)")
set(IR_ROOT "/opt/instantReality" CACHE PATH "Path to InstantReality")

include_directories("${IR_ROOT}/include" "${DTK_ROOT}/include")
link_directories("${IR_ROOT}/lib" "${DTK_ROOT}/lib")


function(MAKE_PLUGIN modName)
    add_library(${modName} SHARED ${modName})
    set_target_properties(${modName} PROPERTIES SUFFIX .iio PREFIX "")
    target_link_libraries(${modName} InstantIO dtk)
endfunction()


file(GLOB mods RELATIVE ${CMAKE_SOURCE_DIR} *.cpp)

string(REGEX REPLACE ".cpp$" "" mods "${mods}")
foreach(mod ${mods})
    MAKE_PLUGIN(${mod})
endforeach(mod)
install(TARGETS ${mods} DESTINATION
    "${CMAKE_INSTALL_PREFIX}/lib/instantReality_plugins")

# cmake string functions sucks.  This would be much shorter in
# any other language.  Their regular expressions of not standard.
file(GLOB navs RELATIVE ${CMAKE_SOURCE_DIR} *-nav.x3d *-nav.wrl)
set(inavs)
# Yes a for loop because CMake blows, string() will not do this.
foreach(nav ${navs})
    if(NOT ${nav} MATCHES "^.*_test-nav[.].*$" )
            set(inavs ${inavs} ${nav})
    endif()
endforeach()
#message(STATUS inavs =========== ${inavs})
if(NOT "${inavs}" STREQUAL "")
    install(FILES ${inavs} DESTINATION "${CMAKE_INSTALL_PREFIX}/share/instantReality_navs")
endif()


file(GLOB engines RELATIVE ${CMAKE_SOURCE_DIR} *-engine.x3d *-engine.wrl)
set(iengines)
foreach(engine ${engines})
    if(NOT ${engine} MATCHES "^.*_test-engine[.].*$" )
            set(iengines ${iengines} ${engine})
    endif()
endforeach()
#message(STATUS iengines =========== "${iengines}")
if(NOT "${iengines}" STREQUAL "")
    install(FILES ${iengines} DESTINATION "${CMAKE_INSTALL_PREFIX}/share/instantReality_engines")
endif()

file(GLOB models RELATIVE ${CMAKE_SOURCE_DIR} *.x3d *.wrl)
set(imodels)
foreach(model ${models})
    if(NOT ${model} MATCHES "^(.*_test.*|.*-engine[.].*|.*-nav[.].*)$" )
            set(imodels ${imodels} ${model})
    endif()
endforeach()
#message(STATUS imodels ========= "${imodels}")
if(NOT "${imodels}" STREQUAL "")
    install(FILES ${imodels} DESTINATION "${CMAKE_INSTALL_PREFIX}/share/models")
endif()

install(FILES InstantPlayer-cave DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
# Next time I write this in GNU make and skip cmake.  This file butt ugly.
