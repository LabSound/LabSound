
project(LavbSoundExamples)

set(src "${LABSOUND_ROOT}/examples/src/ExamplesMain.cpp")

add_executable(example ${src})

_set_Cxx17(example)
_set_compile_options(example)

target_link_libraries(example LabSound)

set_target_properties(example PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set_property(TARGET example PROPERTY FOLDER "examples")

install (TARGETS example RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)
