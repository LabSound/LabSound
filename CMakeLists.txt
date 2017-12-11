
cmake_minimum_required(VERSION 3.4)

set(LABSOUND_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# enforce an out-of-build root.
# CBB: control with a cached variable
set(LOCAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../local")

# override cmake install prefix if it's not set,
# per: http://public.kitware.com/pipermail/cmake/2010-December/041135.html
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX ${LOCAL_ROOT} CACHE PATH "Install set to local" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake/defaults
    ${CMAKE_SOURCE_DIR}/cmake/modules
    ${CMAKE_SOURCE_DIR}/cmake/macros)

include(Utilities)
include(CXXDefaults)
include(Packages)
include(cmake/libnyquist.cmake)
include(cmake/LabSound.cmake)
include(cmake/examples.cmake)

set(CMAKE_BUILD_TYPE Release)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_definitions(${_LAB_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_LAB_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
