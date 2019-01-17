
#-------------------------------------------------------------------------------

include(CXXHelpers)

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

_set_cxx_14(libopus)
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

target_compile_definitions(libopus PRIVATE OPUS_BUILD)
target_compile_definitions(libopus PRIVATE USE_ALLOCA)

set_target_properties(libopus
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(libopus PROPERTIES OUTPUT_NAME_DEBUG libopus_d)

install(TARGETS libopus
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

# folders

source_group(src\\ FILES ${third_opus_src})


#-------------------------------------------------------------------------------

project(libwavpack)

file(GLOB third_wavpack_src
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/wavpack/include/wavpack.h"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/wavpack/src/*.c"
)

add_library(libwavpack STATIC ${third_wavpack_src})

_set_compile_options(libwavpack)

target_include_directories(libwavpack PUBLIC
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/wavpack/include)

set_target_properties(libwavpack
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(libwavpack PROPERTIES OUTPUT_NAME_DEBUG libwavpack_d)

install(TARGETS libwavpack
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

# folders

source_group(src\\ FILES ${third_wavpack_src})

#-------------------------------------------------------------------------------

project(libnyquist)

file(GLOB third_nyquist_h   "${LABSOUND_ROOT}/third_party/libnyquist/include/libnyquist/*")
file(GLOB third_nyquist_src "${LABSOUND_ROOT}/third_party/libnyquist/src/*")

add_library(libnyquist STATIC
    ${third_nyquist_h} ${third_nyquist_src})

_set_cxx_14(libnyquist)
_set_compile_options(libnyquist)

if(WIN32)
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

if(MSVC_IDE)
    # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
    #set_target_properties(libnyquist PROPERTIES PREFIX "../")
    set_target_properties(libnyquist PROPERTIES IMPORT_PREFIX "../")
endif()

set_target_properties(libnyquist PROPERTIES OUTPUT_NAME_DEBUG libnyquist_d)

set_target_properties(libnyquist
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(libnyquist
    libopus libwavpack)

install(TARGETS libnyquist
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

# folders

source_group(include\\libnyquist FILES ${third_nyquist_h})
source_group(src FILES ${third_nyquist_src})
