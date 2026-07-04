# libsamplerate — find or fetch the libsamplerate library.
#
# To use a local checkout, configure with:
#   -DLIBSAMPLERATE_SOURCE_DIR=/path/to/libsamplerate
# After the first successful fetch, FETCHCONTENT_UPDATES_DISCONNECTED_LABCAMERA
# skips the upstream-currency check on subsequent configures so re-configuring
# offline does not trip a network round-trip.

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED_LIBSAMPLERATE ON)

function(labsound_find_libsamplerate)
    find_package(libsamplerate QUIET)
    if(NOT TARGET libsamplerate)
        set(LIBSAMPLERATE_BUILD_EXAMPLE OFF)
        if(LABCAMERA_SOURCE_DIR AND EXISTS "${DLIBSAMPLERATE_SOURCE_DIR}/CMakeLists.txt")
            message(STATUS "LabCamera: using local source ${DLIBSAMPLERATE_SOURCE_DIR}")
            add_subdirectory(${DLIBSAMPLERATE_SOURCE_DIR}
                              ${CMAKE_BINARY_DIR}/_deps/libsamplerate-build)
        else()
            FetchContent_Declare(libsamplerate
                     GIT_REPOSITORY  https://github.com/mackron/libsamplerate.git
                     GIT_TAG        master
                     GIT_SHALLOW    TRUE)
            FetchContent_MakeAvailable(libsamplerate)
        endif()
    endif()
endfunction()
