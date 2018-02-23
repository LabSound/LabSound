
project(LabSoundExamples)

set(src "${LABSOUND_ROOT}/examples/src/ExamplesMain.cpp")

add_executable(example ${src})

_set_Cxx17(example)
_set_compile_options(example)


if (APPLE)
    set(DARWIN_LIBS
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework Accelerate"
        "-framework CoreAudio"
        "-framework Cocoa")
ENDIF (APPLE)

target_link_libraries(example LabSound ${DARWIN_LIBS})

set_target_properties(example PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set_property(TARGET example PROPERTY FOLDER "examples")

install (TARGETS example RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)
