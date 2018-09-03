
function(_add_define definition)
    list(APPEND _LAB_CXX_DEFINITIONS "-D${definition}")
    set(_LAB_CXX_DEFINITIONS ${_LAB_CXX_DEFINITIONS} PARENT_SCOPE)
endfunction()

function(_disable_warning flag)
    if(MSVC)
        list(APPEND _LAB_CXX_WARNING_FLAGS "/wd${flag}")
    else()
        list(APPEND _LAB_CXX_WARNING_FLAGS "-Wno-${flag}")
    endif()
    set(_LAB_CXX_WARNING_FLAGS ${_LAB_CXX_WARNING_FLAGS} PARENT_SCOPE)
endfunction()

function(_set_Cxx17 proj)
	target_compile_features(${proj} INTERFACE cxx_std_17)
endfunction()

# TODO this should be an option on linux
if (UNIX AND NOT APPLE)
    set(LABSOUND_JACK 0)
    set(LABSOUND_ASOUND 1)
    set(LABSOUND_PULSE 1)
endif()

function(_set_compile_options proj)
    if (WIN32)
        target_compile_options(${proj} PRIVATE /arch:AVX /Zi)
        # TODO: These vars are for libniquist and should be set in the find libynquist script.
        target_compile_definitions(${proj} PRIVATE HAVE_STDINT_H=1 HAVE_SINF=1)
    endif()
    if (UNIX AND NOT APPLE)
        target_link_libraries(${proj} pthread)
        target_compile_options(${proj} PRIVATE -fPIC)
        target_compile_definitions(${proj} PRIVATE WEBAUDIO_KISSFFT=1)
        if (LABSOUND_JACK)
            target_link_libraries(${proj} jack)
            target_compile_definitions(${proj} PRIVATE __UNIX_JACK__=1 LABSOUND_JACK=1)
        endif()
        if (LABSOUND_PULSE)
            target_link_libraries(${proj} pulse pulse-simple)
            target_compile_definitions(${proj} PRIVATE __LINUX_PULSE__=1 LABSOUND_PULSE=1)
        endif()
        if (LABSOUND_ASOUND)
            target_link_libraries(${proj} asound)
            target_compile_definitions(${proj} PRIVATE __LINUX_ASOUND__=1 LABSOUND_ASOUND=1)
        endif()
        # TODO: These vars are for libniquist and should be set in the find libynquist script.
        # TODO: libnyquist's loadabc calls getenv and setenv. That's undesirable.
        target_compile_definitions(${proj} PRIVATE HAVE_STDINT_H=1 HAVE_SETENV=1 HAVE_SINF=1)
    endif()

endfunction()
