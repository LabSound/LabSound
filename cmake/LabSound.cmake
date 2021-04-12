
# LabSound
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (C) 2020, The LabSound Authors. All rights reserved.
#
# Will create a target named LabSound

set(LIBSAMPLERATE_INSTALL OFF)
add_subdirectory("${LABSOUND_ROOT}/third_party/libsamplerate")

file(GLOB labsnd_core_h     "${LABSOUND_ROOT}/include/LabSound/core/*")
file(GLOB labsnd_extended_h "${LABSOUND_ROOT}/include/LabSound/extended/*")
file(GLOB labsnd_core       "${LABSOUND_ROOT}/src/core/*")
file(GLOB labsnd_extended   "${LABSOUND_ROOT}/src/extended/*")
file(GLOB labsnd_int_h      "${LABSOUND_ROOT}/src/internal/*")
file(GLOB labsnd_int_src    "${LABSOUND_ROOT}/src/internal/src/*")

# backend selection

if (IOS)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" ON)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" OFF)
elseif (APPLE)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" OFF)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" ON)
elseif (WIN32)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" OFF)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" ON)
elseif (ANDROID)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" ON)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" OFF)
elseif (UNIX)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" OFF)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" ON)
else ()
    message(FATAL, " Untested platform. Please try miniaudio and report results on the LabSound issues page")
endif()

if (LABSOUND_USE_MINIAUDIO AND LABSOUND_USE_RTAUDIO)
    message(FATAL, " Specify only one backend")
elseif(NOT LABSOUND_USE_MINIAUDIO AND NOT LABSOUND_USE_RTAUDIO)
    message(FATAL, " Specify at least one backend")
endif()

if (LABSOUND_USE_MINIAUDIO)
    message(STATUS "Using miniaudio backend")
    if (IOS)
        set(labsnd_backend
            "${LABSOUND_ROOT}/src/backends/miniaudio/AudioDevice_Miniaudio.mm"
            "${LABSOUND_ROOT}/src/backends/miniaudio/AudioDevice_Miniaudio.h"
            "${LABSOUND_ROOT}/src/backends/miniaudio/miniaudio.h"
        )
    else()
        set(labsnd_backend
            "${LABSOUND_ROOT}/src/backends/miniaudio/AudioDevice_Miniaudio.cpp"
            "${LABSOUND_ROOT}/src/backends/miniaudio/AudioDevice_Miniaudio.h"
            "${LABSOUND_ROOT}/src/backends/miniaudio/miniaudio.h"
        )
    endif()
elseif (LABSOUND_USE_RTAUDIO)
    message(STATUS "Using RtAudio backend")
    set(labsnd_backend
        "${LABSOUND_ROOT}/src/backends/RtAudio/AudioDevice_RtAudio.cpp"
        "${LABSOUND_ROOT}/src/backends/RtAudio/AudioDevice_RtAudio.h"
        "${LABSOUND_ROOT}/src/backends/RtAudio/RtAudio.cpp"
        "${LABSOUND_ROOT}/src/backends/RtAudio/RtAudio.h"
    )
endif()


# FFT
if (IOS)
    set(labsnd_fft_src "${LABSOUND_ROOT}/src/backends/FFTFrameAppleAcclerate.cpp")
elseif (APPLE)
    set(labsnd_fft_src "${LABSOUND_ROOT}/src/backends/FFTFrameAppleAcclerate.cpp")
elseif (WIN32)
    file(GLOB labsnd_fft_src "${LABSOUND_ROOT}/third_party/kissfft/src/*")
elseif (UNIX)
    file(GLOB labsnd_fft_src "${LABSOUND_ROOT}/third_party/kissfft/src/*")
endif()

# TODO ooura or kissfft? benchmark and choose. Then benchmark vs FFTFrameAppleAcclerate
set(ooura_src
    "${LABSOUND_ROOT}/third_party/ooura/src/fftsg.cpp"
    "${LABSOUND_ROOT}/third_party/ooura/fftsg.h")

add_library(LabSound STATIC
    "${LABSOUND_ROOT}/include/LabSound/LabSound.h"
    ${labsnd_core_h}     ${labsnd_core}
    ${labsnd_extended_h} ${labsnd_extended}
    ${labsnd_int_h}      ${labsnd_int_src}
    ${labsnd_backend}
    ${labsnd_fft_src}
    ${ooura_src}
 )

if (APPLE)
    set_target_properties(LabSound PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER com.Lab.LabSound
        VERSION 0.13.0
        SOVERSION 1.0.0
        PUBLIC_HEADER "${labsnd_core_h} ${labsnd_extended_h}"
    )
    set_property(SOURCE ${labsnd_core_h}
        PROPERTY MACOSX_PACKAGE_LOCATION Headers/LabSound/core)
    set_property(SOURCE ${labsnd_extended_h}
        PROPERTY MACOSX_PACKAGE_LOCATION Headers/LabSound/extended)
endif()

_set_cxx_14(LabSound)
_set_compile_options(LabSound)

target_include_directories(LabSound PUBLIC
    ${LABSOUND_ROOT}/include)

target_include_directories(LabSound PRIVATE
    ${LABSOUND_ROOT}/src
    ${LABSOUND_ROOT}/src/internal
    ${LABSOUND_ROOT}/third_party
    ${LABSOUND_ROOT}/third_party/libnyquist/include)

if (MSVC_IDE)
    # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
    set_target_properties(LabSound PROPERTIES IMPORT_PREFIX "../")
endif()

set_target_properties(LabSound
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(LabSound PROPERTIES OUTPUT_NAME_DEBUG LabSound_d)

if (WIN32)
    target_compile_definitions(LabSound PRIVATE __WINDOWS_WASAPI__=1)
elseif (APPLE)
    target_compile_definitions(LabSound PRIVATE __MACOSX_CORE__=1)
else()
    set(LABSOUND_PLATFORM_LINK_LIBRARIES dl)
    if (LABSOUND_JACK)
        target_compile_definitions(LabSound PRIVATE __UNIX_JACK__=1)
        set(LIBNYQUIST_JACK ON)
    elseif (LABSOUND_PULSE)
        target_compile_definitions(LabSound PRIVATE __LINUX_PULSE__=1)
        set(LIBNYQUIST_PULSE ON)
    elseif (LABSOUND_ASOUND)
        target_compile_definitions(LabSound PRIVATE __LINUX_ALSA__=1)
        set(LIBNYQUIST_ASOUND ON)
    else()
        message(FATAL, "On Linux, one of LABSOUND_JACK, LABSOUND_PULSE, or LABSOUND_ASOUND must be set ON.")
    endif()
endif()

target_link_libraries(LabSound
    libnyquist libwavpack
    samplerate
    ${LABSOUND_PLATFORM_LINK_LIBRARIES})

install(TARGETS LabSound
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    FRAMEWORK DESTINATION lib
    RUNTIME DESTINATION bin)

install(FILES "${LABSOUND_ROOT}/include/LabSound/LabSound.h"
    DESTINATION include/LabSound)
install(FILES ${labsnd_core_h}
    DESTINATION include/LabSound/core)
install(FILES ${labsnd_extended_h}
    DESTINATION include/LabSound/extended)
install(DIRECTORY
    assets/hrtf
    assets/images
    assets/impulse
    assets/json
    assets/pd
    assets/samples
    DESTINATION share/LabSound)

source_group(include FILES "${LABSOUND_ROOT}/include/LabSound")
source_group(include\\core FILES ${labsnd_core_h})
source_group(include\\extended FILES ${labsnd_extended_h})
source_group(src\\backends FILES ${labsnd_backend})
source_group(src\\core FILES ${labsnd_core})
source_group(src\\extended FILES ${labsnd_extended})
source_group(src\\internal FILES ${labsnd_int_h})
source_group(src\\internal\\src FILES ${labsnd_int_src})
source_group(third_party\\kissfft FILES ${third_kissfft})
source_group(third_party\\ooura FILES ${ooura_src})
source_group(third_party\\rtaudio FILES ${third_rtaudio})

configure_file("${LABSOUND_ROOT}/cmake/LabSoundConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/LabSoundConfig.cmake" @ONLY)
install(FILES
  "${PROJECT_BINARY_DIR}/LabSoundConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

add_library(Lab::Sound ALIAS LabSound)
