
project(LabSoundExamples)

set(src "${LABSOUND_ROOT}/examples/src/ExamplesMain.cpp")

add_executable(LabSoundExample ${src})

set_cxx_version(LabSoundExample)
_set_compile_options(LabSoundExample)


if(APPLE)
    set(DARWIN_LIBS
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework Accelerate"
        "-framework CoreAudio"
        "-framework Cocoa")
ENDIF(APPLE)

target_link_libraries(LabSoundExample LabSound ${DARWIN_LIBS})

set_target_properties(LabSoundExample PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set_property(TARGET LabSoundExample PROPERTY FOLDER "examples")

install(TARGETS LabSoundExample
    RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)
