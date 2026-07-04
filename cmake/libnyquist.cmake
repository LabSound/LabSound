# libnyquist — find or fetch the libnyquist library.
#
# To use a local checkout, configure with:
#   -DLIBNYQUIST_SOURCE_DIR=/path/to/libnyquist
# After the first successful fetch, FETCHCONTENT_UPDATES_DISCONNECTED_LABCAMERA
# skips the upstream-currency check on subsequent configures so re-configuring
# offline does not trip a network round-trip.

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED_LIBNYQUIST ON)

function(labsound_find_libnyquist)
    find_package(libnyquist QUIET)
    if(NOT TARGET libnyquist)
        set(LIBNYQUIST_BUILD_EXAMPLE OFF)
        if(LABNYQUIST_SOURCE_DIR AND EXISTS "${DLIBNYQUIST_SOURCE_DIR}/CMakeLists.txt")
            message(STATUS "LabCamera: using local source ${DLIBNYQUIST_SOURCE_DIR}")
            add_subdirectory(${DLIBNYQUIST_SOURCE_DIR}
                             ${CMAKE_BINARY_DIR}/_deps/libnyquist-build)
        else()
            FetchContent_Declare(libnyquist
                    GIT_REPOSITORY https://github.com/LabSound/libnyquist.git
                    GIT_TAG        master)
            FetchContent_MakeAvailable(libnyquist)
        endif()
    endif()
endfunction()
