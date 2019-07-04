
set(labsound_examples_src
    "${LABSOUND_ROOT}/examples/src/ExampleBaseApp.cpp"
    "${LABSOUND_ROOT}/examples/src/ExampleBaseApp.h"
    "${LABSOUND_ROOT}/examples/src/ExamplesMain.cpp")
file(GLOB labsound_examples_hdr "${LABSOUND_ROOT}/examples/*.h")

add_executable(LabSoundExample ${labsound_examples_src} ${labsound_examples_hdr})

_set_cxx_14(LabSoundExample)
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
                      RUNTIME_OUTPUT_DIRECTORY bin)

set_property(TARGET LabSoundExample PROPERTY FOLDER "examples")

install(TARGETS LabSoundExample
    RUNTIME DESTINATION bin)
