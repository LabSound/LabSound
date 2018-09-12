
cmake_minimum_required(VERSION 3.11)

include_guard()

set(LABSOUND_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# override cmake install prefix if it's not set,
# per: http://public.kitware.com/pipermail/cmake/2010-December/041135.html

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX ${LOCAL_ROOT} CACHE PATH "Install set to local" FORCE)
ENDIF()

set(CMAKE_MODULE_PATH
    ${LABSOUND_ROOT}/cmake/defaults
    ${LABSOUND_ROOT}/cmake/modules
    ${LABSOUND_ROOT}/cmake/macros)

include(Utilities)
include(CXXDefaults)
include(cmake/libnyquist.cmake)
include(cmake/LabSound.cmake)
include(cmake/examples.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_definitions(${_LAB_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_LAB_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
