# miniaudio — find or fetch the miniaudio library.
#
# To use a local checkout, configure with:
#   -DMINIAUDIO_SOURCE_DIR=/path/to/miniaudio
# After the first successful fetch, FETCHCONTENT_UPDATES_DISCONNECTED_MINIAUDIO
# skips the upstream-currency check on subsequent configures so re-configuring
# offline does not trip a network round-trip.

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED_MINIAUDIO ON)

function(labsound_find_miniaudio)
    find_package(miniaudio QUIET)
    if(NOT TARGET miniaudio)
        set(MINIAUDIO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(MINIAUDIO_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
        set(MINIAUDIO_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
        if(MINIAUDIO_SOURCE_DIR AND EXISTS "${MINIAUDIO_SOURCE_DIR}/CMakeLists.txt")
            message(STATUS "miniaudio: using local source ${MINIAUDIO_SOURCE_DIR}")
            add_subdirectory(${MINIAUDIO_SOURCE_DIR}
                              ${CMAKE_BINARY_DIR}/_deps/miniaudio-build)
            set(miniaudio_SOURCE_DIR "${MINIAUDIO_SOURCE_DIR}" PARENT_SCOPE)
        else()
            FetchContent_Declare(miniaudio
                     GIT_REPOSITORY  https://github.com/mackron/miniaudio.git
                     GIT_TAG        master
                     GIT_SHALLOW    TRUE)
            FetchContent_MakeAvailable(miniaudio)
            set(miniaudio_SOURCE_DIR "${miniaudio_SOURCE_DIR}" PARENT_SCOPE)
        endif()
    endif()
endfunction()
