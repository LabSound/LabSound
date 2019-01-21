

function(_add_define definition)
    list(APPEND _LAB_CXX_DEFINITIONS "-D${definition}")
    set(_LAB_CXX_DEFINITIONS ${_LAB_CXX_DEFINITIONS} PARENT_SCOPE)
endfunction()

if (CMAKE_COMPILER_IS_GNUCXX)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
elseif(MSVC)
    # Disable warning C4996 regarding fopen(), strcpy(), etc.
    _add_define("_CRT_SECURE_NO_WARNINGS")

    # Disable warning C4996 regarding unchecked iterators for std::transform,
    # std::copy, std::equal, et al.
    _add_define("_SCL_SECURE_NO_WARNINGS")

    # Make sure WinDef.h does not define min and max macros which
    # will conflict with std::min() and std::max().
    _add_define("NOMINMAX")
endif()

function(_disable_warning flag)
    if(MSVC)
        list(APPEND _LAB_CXX_WARNING_FLAGS "/wd${flag}")
    else()
        list(APPEND _LAB_CXX_WARNING_FLAGS "-Wno-${flag}")
    endif()
    set(_LAB_CXX_WARNING_FLAGS ${_LAB_CXX_WARNING_FLAGS} PARENT_SCOPE)
endfunction()

function(_set_compile_options proj)
    if(WIN32)
        target_compile_options(${proj} PRIVATE /arch:AVX /Zi)
        target_compile_definitions(${proj} PRIVATE __WINDOWS_WASAPI__=1)
        # TODO: These vars are for libniquist and should be set in the find libynquist script.
        target_compile_definitions(${proj} PRIVATE HAVE_STDINT_H=1 HAVE_SINF=1)
    elseif(APPLE)
    elseif(UNIX)
        target_link_libraries(${proj} pthread)
        target_compile_options(${proj} PRIVATE -fPIC)
        target_compile_definitions(${proj} PRIVATE WEBAUDIO_KISSFFT=1)
        if(LABSOUND_JACK)
            target_link_libraries(${proj} jack)
            target_compile_definitions(${proj} PRIVATE __UNIX_JACK__=1)
        endif()
        if(LABSOUND_PULSE)
            target_link_libraries(${proj} pulse pulse-simple)
            target_compile_definitions(${proj} PRIVATE __LINUX_PULSE__=1)
        endif()
        if(LABSOUND_ASOUND)
            target_link_libraries(${proj} asound)
            target_compile_definitions(${proj} PRIVATE __LINUX_ASOUND__=1)
        endif()
        # TODO: These vars are for libniquist and should be set in the find libynquist script.
        # TODO: libnyquist's loadabc calls getenv and setenv. That's undesirable.
        target_compile_definitions(${proj} PRIVATE HAVE_STDINT_H=1 HAVE_SETENV=1 HAVE_SINF=1)
    endif()

endfunction()

function(_set_cxx_14 proj)
    target_compile_features(${proj} INTERFACE cxx_std_14)
endfunction()

function(source_file fname)
    if(IS_ABSOLUTE ${fname})
        target_sources(${PROJECT_NAME} PRIVATE ${fname})
    else()
        target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${fname})
    endif()
endfunction()

function(include_dir fpath)
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${fpath})
endfunction()
