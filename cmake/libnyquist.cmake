
#-------------------------------------------------------------------------------

include(CXXhelpers)

project(libopus)

file(GLOB third_opus_src
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/*.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/src/*.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/src/*.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/*.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/float/*.c"
)
list(FILTER third_opus_src EXCLUDE REGEX ".*demo.c$")

add_library(libopus STATIC ${third_opus_src})

set_cxx_version(libopus)
_set_compile_options(libopus)

if (WIN32)
    _disable_warning(4244)
    _disable_warning(4018)
endif()

target_include_directories(libopus PRIVATE
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/libogg/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/src/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/float)

if (MSVC_IDE)
    # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
    #set_target_properties(libnyquist PROPERTIES PREFIX "../")
    set_target_properties(libopus PROPERTIES IMPORT_PREFIX "../")
endif()

target_compile_definitions(libopus PRIVATE OPUS_BUILD)
target_compile_definitions(libopus PRIVATE USE_ALLOCA)

set_target_properties(libopus
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(libopus PROPERTIES OUTPUT_NAME_DEBUG libopus_d)

install (TARGETS libopus
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install (TARGETS libopus DESTINATION lib)

# folders

source_group(src\\ FILES ${third_opus_src})


#-------------------------------------------------------------------------------

project(libnyquist)

file(GLOB third_nyquist_h   "${LABSOUND_ROOT}/third_party/libnyquist/include/libnyquist/*")
file(GLOB third_nyquist_src "${LABSOUND_ROOT}/third_party/libnyquist/src/*")

add_library(libnyquist STATIC
    ${third_nyquist_h} ${third_nyquist_src})

set_cxx_version(libnyquist)
_set_compile_options(libnyquist)

if (WIN32)
    target_compile_definitions(libnyquist PRIVATE MODPLUG_STATIC)

    _disable_warning(4244)
    _disable_warning(4018)
endif()

target_include_directories(libnyquist PRIVATE
    ${LABSOUND_ROOT}/third_party/libnyquist/include
    ${LABSOUND_ROOT}/third_party/libnyquist/include/libnyquist
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/FLAC/src/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/libogg/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/libvorbis/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/libvorbis/src
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/musepack/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/src/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/float
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/wavpack/include
    ${LABSOUND_ROOT}/third_party/libnyquist/src)

if (MSVC_IDE)
    # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
    #set_target_properties(libnyquist PROPERTIES PREFIX "../")
    set_target_properties(libnyquist PROPERTIES IMPORT_PREFIX "../")
endif()

set_target_properties(libnyquist PROPERTIES OUTPUT_NAME_DEBUG libnyquist_d)

set_target_properties(libnyquist
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(libnyquist libopus)

install (TARGETS libnyquist
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install (TARGETS libnyquist DESTINATION lib)

# folders

source_group(include\\libnyquist FILES ${third_nyquist_h})
source_group(src FILES ${third_nyquist_src})
source_group(third_party\\musepack\\ FILES ${third_musepack_enc} ${third_musepack_dec})
source_group(third_party\\wavpack\\ FILES ${third_wavpack})
source_group(third_party\\opus\\ FILES ${third_opus_src})
