cmake_minimum_required(VERSION 3.14)
project(labsoundpy)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python
find_package(Python 3.7 REQUIRED COMPONENTS Interpreter Development)

# Find nanobind
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nanobind/CMakeLists.txt")
    message(STATUS "Downloading nanobind...")
    execute_process(
        COMMAND git clone --depth 1 https://github.com/wjakob/nanobind.git third_party/nanobind
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Add nanobind
add_subdirectory(third_party/nanobind)

# Find LabSound
# Assuming LabSound is built and installed in a standard location
# or that we're building within the LabSound repository
if(NOT DEFINED LABSOUND_DIR)
    set(LABSOUND_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

# Include LabSound headers
include_directories(
    ${LABSOUND_DIR}/include
    ${LABSOUND_DIR}/src
)

# Define the extension module
nanobind_add_module(
    _core                           # Module name
    src/core.cpp                    # Main binding file
    src/audio_context.cpp           # AudioContext bindings
    src/audio_node.cpp              # AudioNode base class bindings
    src/audio_param.cpp             # AudioParam bindings
    src/nodes/oscillator_node.cpp   # OscillatorNode bindings
    src/nodes/gain_node.cpp         # GainNode bindings
    # Add other node bindings as they are implemented
)

# Link against LabSound libraries
target_link_libraries(_core PRIVATE 
    ${LABSOUND_DIR}/build/lib/libLabSound.a
    ${LABSOUND_DIR}/build/lib/libLabSoundCore.a
)

# Install the extension module
install(TARGETS _core DESTINATION labsound)
