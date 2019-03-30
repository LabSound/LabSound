
cmake_minimum_required(VERSION 3.10)

include_guard()

set(LABSOUND_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

if (LABSOUND_ASOUND)
    set(LIBNYQUIST_ASOUND ON)
elseif (LABSOUND_JACK)
    set(LIBNYQUIST_JACK ON)
elseif (LABSOUND_PULSE)
    set(LIBNYQUIST_PULSE ON)
endif()

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(third_party/libnyquist)

include(cmake/common.cmake)
include(cmake/LabSound.cmake)
include(cmake/examples.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_definitions(${_LAB_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_LAB_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
