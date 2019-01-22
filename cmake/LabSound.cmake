
project(LabSound)

file(GLOB labsnd_core_h     "${LABSOUND_ROOT}/include/LabSound/core/*")
file(GLOB labsnd_core       "${LABSOUND_ROOT}/src/core/*")
file(GLOB labsnd_extended_h "${LABSOUND_ROOT}/include/LabSound/extended/*")
file(GLOB labsnd_extended   "${LABSOUND_ROOT}/src/extended/*")
file(GLOB labsnd_int_h      "${LABSOUND_ROOT}/src/internal/*")
file(GLOB labsnd_int_src    "${LABSOUND_ROOT}/src/internal/src/*")
file(GLOB third_kissfft     "${LABSOUND_ROOT}/third_party/kissfft/src/*")

if (APPLE)
    file(GLOB labsnd_backend "${LABSOUND_ROOT}/src/backends/darwin/*")
elseif (WIN32)
    file(GLOB labsnd_backend "${LABSOUND_ROOT}/src/backends/windows/*")
    set(third_rtaudio "${LABSOUND_ROOT}/third_party/rtaudio/src/RtAudio.cpp")
elseif (UNIX)
    file(GLOB labsnd_backend "${LABSOUND_ROOT}/src/backends/linux/*")
    set(third_rtaudio "${LABSOUND_ROOT}/third_party/rtaudio/src/RtAudio.cpp")
endif()

add_library(LabSound STATIC
    ${labsnd_core_h}     ${labsnd_core}
    ${labsnd_extended_h} ${labsnd_extended}
    ${labsnd_int_h}      ${labsnd_int_src}
    ${labsnd_backend}
    ${third_rtaudio}
    ${third_kissfft}
 )

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
else()
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

target_link_libraries(LabSound libnyquist libopus libwavpack)

install(TARGETS LabSound
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install(FILES ${labsnd_core_h}
        DESTINATION include/LabSound/core)
install(FILES ${labsnd_extended_h}
        DESTINATION include/LabSound/extended)

source_group(include\\core FILES ${labsnd_core_h})
source_group(include\\extended FILES ${labsnd_extended_h})
source_group(src\\backends FILES ${labsnd_backend})
source_group(src\\core FILES ${labsnd_core})
source_group(src\\extended FILES ${labsnd_extended})
source_group(src\\internal FILES ${labsnd_int_h})
source_group(src\\internal\\src FILES ${labsnd_int_src})
source_group(third_party\\kissfft FILES ${third_kissfft})
source_group(third_party\\rtaudio FILES ${third_rtaudio})
