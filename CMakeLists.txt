
cmake_minimum_required(VERSION 3.15)

# Allow overriding of MSVC runtime library linkage via the standard CMake variable
# e.g. -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded"
cmake_policy(SET CMP0091 NEW)

include_guard()

project(LabSound)

set(LABSOUND_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_INSTALL_MESSAGE LAZY)

# default backend set up

if (WIN32)
elseif (APPLE)
elseif (ANDROID)
else()
    if (NOT LABSOUND_JACK AND NOT LABSOUND_PULSE AND NOT LABSOUND_ASOUND)
        message(STATUS "No Linux backend specified, defaulting to Pulse.")
        set(LABSOUND_PULSE ON)
    endif()
endif()

if (LABSOUND_ASOUND)
    set(LIBNYQUIST_ASOUND ON)
elseif (LABSOUND_JACK)
    set(LIBNYQUIST_JACK ON)
elseif (LABSOUND_PULSE)
    set(LIBNYQUIST_PULSE ON)
endif()

set(CMAKE_CXX_STANDARD 14)

# turn off building libnyquist samples
set(LIBNYQUIST_BUILD_EXAMPLE OFF)
add_subdirectory(third_party/libnyquist)

include(cmake/LabSound.cmake)

if (NOT ANDROID AND NOT IOS)
    include(cmake/examples.cmake)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions(${_LAB_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_LAB_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
