# miniaudio — find or fetch the miniaudio library.
#
# To use a local checkout, configure with:
#   -DMINIAUDIO_SOURCE_DIR=/path/to/miniaudio
# After the first successful fetch, FETCHCONTENT_UPDATES_DISCONNECTED_LABCAMERA
# skips the upstream-currency check on subsequent configures so re-configuring
# offline does not trip a network round-trip.

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED_MINIAUDIO ON)

function(labsound_find_miniaudio)
    find_package(miniaudio QUIET)
    if(NOT TARGET miniaudio)
        set(MINIAUDIO_BUILD_EXAMPLE OFF)
        if(LABCAMERA_SOURCE_DIR AND EXISTS "${DMINIAUDIO_SOURCE_DIR}/CMakeLists.txt")
            message(STATUS "LabCamera: using local source ${DMINIAUDIO_SOURCE_DIR}")
            add_subdirectory(${DMINIAUDIO_SOURCE_DIR}
                              ${CMAKE_BINARY_DIR}/_deps/miniaudio-build)
        else()
            FetchContent_Declare(miniaudio
                     GIT_REPOSITORY  https://github.com/mackron/miniaudio.git
                     GIT_TAG        master
                     GIT_SHALLOW    TRUE)
            FetchContent_MakeAvailable(miniaudio)
        endif()
    endif()
endfunction()
