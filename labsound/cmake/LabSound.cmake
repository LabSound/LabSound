
project(LabSound)

file(GLOB labsnd_core_h     "${LABSOUND_ROOT}/include/LabSound/core/*")
file(GLOB labsnd_core       "${LABSOUND_ROOT}/src/core/*")
file(GLOB labsnd_extended_h "${LABSOUND_ROOT}/include/LabSound/extended/*")
file(GLOB labsnd_extended   "${LABSOUND_ROOT}/src/extended/*")
file(GLOB labsnd_int_h      "${LABSOUND_ROOT}/src/internal/*")
file(GLOB labsnd_int_src    "${LABSOUND_ROOT}/src/internal/src/*")
file(GLOB third_kissfft     "${LABSOUND_ROOT}/third_party/kissfft/src/*")

if (APPLE)
else()
	set(third_rtaudio "${LABSOUND_ROOT}/third_party/rtaudio/src/RtAudio.cpp")
endif()

if (APPLE)
	file(GLOB labsnd_int_platform     "${LABSOUND_ROOT}/src/internal/mac/*")
	file(GLOB labsnd_intsrc_platform  "${LABSOUND_ROOT}/src/internal/src/mac/*")
elseif (WIN32)
	file(GLOB labsnd_int_platform     "${LABSOUND_ROOT}/src/internal/win/*")
	file(GLOB labsnd_intsrc_platform  "${LABSOUND_ROOT}/src/internal/src/win/*")
elseif (UNIX)
	file(GLOB labsnd_int_platform     "${LABSOUND_ROOT}/src/internal/linux/*")
	file(GLOB labsnd_intsrc_platform  "${LABSOUND_ROOT}/src/internal/src/linux/*")
endif()

add_library(LabSound STATIC
	${labsnd_core_h} ${labsnd_core}
	${labsnd_extended_h} ${labsnd_extended}
	${labsnd_int_h} ${labsnd_int_src}
	${labsnd_int_platform} ${labsnd_intsrc_platform}
 	${third_rtaudio} ${third_kissfft}
)

_set_Cxx17(LabSound)
_set_compile_options(LabSound)

target_include_directories(LabSound PUBLIC
    ${LABSOUND_ROOT}/include)
target_include_directories(LabSound PRIVATE
    ${LABSOUND_ROOT}/src
    ${LABSOUND_ROOT}/src/internal
    ${LABSOUND_ROOT}/third_party
    ${LABSOUND_ROOT}/../node_modules/native-video-deps/include)

if (MSVC_IDE)
    # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
    #set_target_properties(LabSound PROPERTIES PREFIX "../")
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
endif()

target_link_libraries(LabSound)

install (TARGETS LabSound
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install (TARGETS LabSound DESTINATION lib)

install(FILES ${labsnd_core_h}
        DESTINATION include/LabSound/core)
install(FILES ${labsnd_extended_h}
        DESTINATION include/LabSound/extended)

source_group(include\\core FILES ${labsnd_core_h})
source_group(include\\extended FILES ${labsnd_extended_h})
source_group(src\\core FILES ${labsnd_core})
source_group(src\\extended FILES ${labsnd_extended})
source_group(src\\internal FILES ${labsnd_int_h})
source_group(src\\internal\\src FILES ${labsnd_int_src})
source_group(third_party\\kissfft FILES ${third_kissfft})
source_group(third_party\\rtaudio FILES ${third_rtaudio})
source_group(src\\internal\\platform FILES ${labsnd_int_platform})
source_group(src\\internal\\platform\\src\\platform FILES ${labsnd_intsrc_platform})
