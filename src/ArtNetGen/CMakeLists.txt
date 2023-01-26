cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SWIG REQUIRED)
#find_package(Boost REQUIRED)

set(ARTNETGEN_WRAP ${CMAKE_CURRENT_BINARY_DIR}/artNetGen_wrap.cc)
set(ARTNETGEN_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/artNetGenExtTclInitVar.cc)

add_custom_command(OUTPUT ${ARTNETGEN_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -I${ARTNETGEN_HOME}/include -I${OPENDB_HOME}/src/swig/tcl -o ${ARTNETGEN_WRAP} ${ARTNETGEN_HOME}/src/artNetGen.i
  WORKING_DIRECTORY ${ARTNETGEN_HOME}/src
  DEPENDS ${ARTNETGEN_HOME}/src/artNetGen.i ${ARTNETGEN_HOME}/include/artnetgen/artNetGen.h
)

add_custom_command(OUTPUT ${ARTNETGEN_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${ARTNETGEN_TCL_INIT} artnetgen_tcl_inits ${ARTNETGEN_HOME}/src/artNetGen.tcl
  WORKING_DIRECTORY ${ARTNETGEN_HOME}/src
  DEPENDS ${ARTNETGEN_HOME}/src/artNetGen.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

############################################################
# Libraries setting 
############################################################

add_library( ArtNetGen 
  src/artNetGen.cpp
  src/MakeArtNetGen.cpp
  src/logger.cpp
  src/netlist.cpp
  src/distribution.cpp
  src/bin.cpp
  src/node.cpp
  src/techMapping.cpp
  src/distMatching.cpp
  src/writeSpec.cpp
  ${ARTNETGEN_WRAP}
  ${ARTNETGEN_TCL_INIT}
)

target_include_directories( ArtNetGen 
  PUBLIC ${ARTNETGEN_HOME}/include
  PRIVATE
  ${ARTNETGEN_HOME}/src
  ${OPENROAD_HOME}/include
  ${OPENDB_HOME}/include
  ${DBSTA_HOME}/include
  ${OPENSTA_HOME}/include

  ${Boost_INCLUDE_DIR}
 )
