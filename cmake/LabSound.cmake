
# LabSound
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (C) 2020, The LabSound Authors. All rights reserved.
#
# Will create a target named LabSound

# backend selection
if (IOS)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" ON)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" OFF)
elseif (APPLE)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" OFF)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" ON)
elseif (WIN32)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" ON)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" OFF)
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
elseif (LABSOUND_USE_RTAUDIO)
    message(STATUS "Using RtAudio backend")
    set(labsnd_backend
        "${LABSOUND_ROOT}/src/backends/RtAudio/AudioDevice_RtAudio.cpp"
        "${LABSOUND_ROOT}/src/backends/RtAudio/AudioDevice_RtAudio.h"
        "${LABSOUND_ROOT}/src/backends/RtAudio/RtAudio.cpp"
        "${LABSOUND_ROOT}/src/backends/RtAudio/RtAudio.h"
    )
endif()

#option(LIBSAMPLERATE_EXAMPLES "" OFF)
#option(LIBSAMPLERATE_INSTALL "" ON)
#option(BUILD_TESTING "" OFF) # suppress testing of libsamplerate
#add_subdirectory("${LABSOUND_ROOT}/third_party/libsamplerate")

file(GLOB labsnd_core_h     "${LABSOUND_ROOT}/include/LabSound/core/*")
file(GLOB labsnd_extended_h "${LABSOUND_ROOT}/include/LabSound/extended/*")
file(GLOB labsnd_core       "${LABSOUND_ROOT}/src/core/*")
file(GLOB labsnd_extended   "${LABSOUND_ROOT}/src/extended/*")
file(GLOB labsnd_int_h      "${LABSOUND_ROOT}/src/internal/*")
file(GLOB labsnd_int_src    "${LABSOUND_ROOT}/src/internal/src/*")

# FFT
if (IOS)
    set(labsnd_fft_src "${LABSOUND_ROOT}/src/internal/FFTFrameAppleAccelerate.cpp")
elseif (APPLE)
    set(labsnd_fft_src "${LABSOUND_ROOT}/src/internal/FFTFrameAppleAccelerate.cpp")
elseif (WIN32)
    file(GLOB labsnd_fft_src "${LABSOUND_ROOT}/third_party/kissfft/src/*")
elseif (UNIX)
    file(GLOB labsnd_fft_src "${LABSOUND_ROOT}/third_party/kissfft/src/*")
endif()

# TODO ooura or kissfft? benchmark and choose. Then benchmark vs FFTFrameAppleAccelerate
set(ooura_src
    "${LABSOUND_ROOT}/third_party/ooura/src/fftsg.cpp"
    "${LABSOUND_ROOT}/third_party/ooura/fftsg.h")

add_library(LabSound STATIC
    "${LABSOUND_ROOT}/include/LabSound/LabSound.h"
    ${labsnd_core_h}     ${labsnd_core}
    ${labsnd_extended_h} ${labsnd_extended}
    ${labsnd_int_h}      ${labsnd_int_src}
    ${labsnd_fft_src}
    ${ooura_src}
 )

 #--- CONFIGURE RTAUDIO
if (NOT IOS)
    add_library(LabSoundRtAudio STATIC
        "${LABSOUND_ROOT}/src/backends/RtAudio/AudioDevice_RtAudio.cpp"
        "${LABSOUND_ROOT}/include/LabSound/backends/AudioDevice_RtAudio.h"
        "${LABSOUND_ROOT}/src/backends/RtAudio/RtAudio.cpp"
        "${LABSOUND_ROOT}/src/backends/RtAudio/RtAudio.h"
    )

    # set the RtAudio backend selector macros
    if (WIN32)
        target_compile_definitions(LabSoundRtAudio PRIVATE __WINDOWS_WASAPI__=1)
    elseif (APPLE)
        target_compile_definitions(LabSoundRtAudio PRIVATE __MACOSX_CORE__=1)
    else()
        if (LABSOUND_JACK)
            target_compile_definitions(LabSoundRtAudio PRIVATE __UNIX_JACK__=1)
        elseif (LABSOUND_PULSE)
            target_compile_definitions(LabSoundRtAudio PRIVATE __LINUX_PULSE__=1)
        elseif (LABSOUND_ASOUND)
            target_compile_definitions(LabSoundRtAudio PRIVATE __LINUX_ALSA__=1)
        endif()
    endif()

endif()

 #--- CONFIGURE MINIAUDIO
 if (APPLE)
    add_library(LabSoundMiniAudio STATIC
        "${LABSOUND_ROOT}/src/backends/miniaudio/AudioDevice_Miniaudio.mm"
        "${LABSOUND_ROOT}/include/LabSound/backends/AudioDevice_Miniaudio.h"
        "${LABSOUND_ROOT}/third_party/miniaudio/miniaudio.h"
    )
else()
    add_library(LabSoundMiniAudio STATIC
        "${LABSOUND_ROOT}/src/backends/miniaudio/AudioDevice_Miniaudio.cpp"
        "${LABSOUND_ROOT}/include/LabSound/backends/AudioDevice_Miniaudio.h"
        "${LABSOUND_ROOT}/third_party/miniaudio/miniaudio.h"
    )
endif()

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

function (configureProj proj)
    install(TARGETS ${proj}
        EXPORT ${proj}Config
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        FRAMEWORK DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    export(TARGETS ${proj}
        NAMESPACE ${proj}::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${proj}Config.cmake")
    install(EXPORT ${proj}Config
        DESTINATION "${CMAKE_INSTALL_PREFIX}/${proj}/cmake"
        NAMESPACE ${proj}:: )

    set_target_properties(${proj} PROPERTIES OUTPUT_NAME_DEBUG ${proj}_d)

    set_target_properties(${proj}
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    set_property(TARGET ${proj} PROPERTY CXX_STANDARD 17)
    if(WIN32)
        if(MSVC)
            # Arch AVX is problematic for many users, so disable it until
            # some reasonable strategy (a separate AVX target?) is determined
            #target_compile_options(${proj} PRIVATE /arch:AVX /Zi)
            target_compile_options(${proj} PRIVATE /Zi)
        endif(MSVC)
        # TODO: These vars are for libnyquist and should be set in the find libynquist script.
        target_compile_definitions(${proj} PRIVATE HAVE_STDINT_H=1 HAVE_SINF=1)
    elseif(APPLE)
    elseif(ANDROID)
        target_compile_options(${proj} PRIVATE -fPIC)
        target_compile_definitions(${proj} PRIVATE USE_KISS_FFT=1)
        target_link_libraries(${proj} PRIVATE OpenSLES)
        if(LABSOUND_JACK)
            target_link_libraries(${proj} PRIVATE jack)
        endif()
        if(LABSOUND_PULSE)
            target_link_libraries(${proj} PRIVATE pulse PRIVATE pulse-simple)
        endif()
        if(LABSOUND_ASOUND)
            target_link_libraries(${proj} PRIVATE asound)
        endif()
        # TODO: These vars are for libnyquist and should be set in the find libynquist script.
        # TODO: libnyquist's loadabc calls getenv and setenv. That's undesirable.
        target_compile_definitions(${proj} PRIVATE HAVE_STDINT_H=1 HAVE_SETENV=1 HAVE_SINF=1)
    elseif(UNIX)
        target_link_libraries(${proj} PRIVATE pthread)
        target_compile_options(${proj} PRIVATE -fPIC)
        target_compile_definitions(${proj} PRIVATE USE_KISS_FFT=1)
        if(LABSOUND_JACK)
            target_link_libraries(${proj} PRIVATE jack)
        endif()
        if(LABSOUND_PULSE)
            target_link_libraries(${proj} PRIVATE pulse PRIVATE pulse-simple)
        endif()
        if(LABSOUND_ASOUND)
            target_link_libraries(${proj} PRIVATE asound)
        endif()
        # TODO: These vars are for libnyquist and should be set in the find libynquist script.
        # TODO: libnyquist's loadabc calls getenv and setenv. That's undesirable.
        target_compile_definitions(${proj} PRIVATE HAVE_STDINT_H=1 HAVE_SETENV=1 HAVE_SINF=1)
    endif()
endfunction()

target_include_directories(LabSound PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
    $<INSTALL_INTERFACE:include>
)
target_include_directories(LabSound PRIVATE
    ${LABSOUND_ROOT}/src
    ${LABSOUND_ROOT}/src/internal
    ${LABSOUND_ROOT}/third_party
    ${LABSOUND_ROOT}/third_party/libsamplerate/include
)

if (NOT IOS)
    target_include_directories(LabSoundRtAudio PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
        $<INSTALL_INTERFACE:include>
    )
    target_include_directories(LabSoundRtAudio PRIVATE
        ${LABSOUND_ROOT}/src
        ${LABSOUND_ROOT}/src/internal
        ${LABSOUND_ROOT}/third_party
        ${LABSOUND_ROOT}/third_party/libnyquist/include)
endif()

target_include_directories(LabSoundMiniAudio PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
    $<INSTALL_INTERFACE:include>
)

target_include_directories(LabSoundMiniAudio PRIVATE
    ${LABSOUND_ROOT}/third_party/miniaudio
    ${LABSOUND_ROOT}/src
    ${LABSOUND_ROOT}/src/internal
    ${LABSOUND_ROOT}/third_party
    ${LABSOUND_ROOT}/third_party/libnyquist/include)

if (MSVC_IDE)
    # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
    set_target_properties(LabSound PROPERTIES IMPORT_PREFIX "../")
endif()

if (LINUX)
    set(LABSOUND_PLATFORM_LINK_LIBRARIES PRIVATE dl)
endif()

target_link_libraries(LabSound
    ${LABSOUND_PLATFORM_LINK_LIBRARIES}
    PRIVATE libnyquist::libnyquist
)

configureProj(LabSound)
configureProj(LabSoundMiniAudio)
if (NOT IOS)
    configureProj(LabSoundRtAudio)
endif()
    
install(FILES "${LABSOUND_ROOT}/include/LabSound/LabSound.h"
    DESTINATION include/LabSound)
install(FILES ${labsnd_core_h}
    DESTINATION include/LabSound/core)
install(FILES ${labsnd_extended_h}
    DESTINATION include/LabSound/extended)
install(FILES
    "${LABSOUND_ROOT}/include/LabSound/backends/AudioDevice_Miniaudio.h"
    "${LABSOUND_ROOT}/include/LabSound/backends/AudioDevice_RtAudio.h"
   DESTINATION include/LabSound/backends)

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

add_library(LabSound::LabSound ALIAS LabSound)
add_library(LabSoundMiniAudio::LabSoundMiniAudio ALIAS LabSoundMiniAudio)
if (NOT IOS)
    add_library(LabSoundRtAudio::LabSoundRtAudio ALIAS LabSoundRtAudio)
endif()
