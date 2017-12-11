
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

function(_set_compile_options proj)
	target_compile_options(${proj} PRIVATE /arch:AVX /Zi )
endfunction()
