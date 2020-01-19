
set(labsound_examples_src
    "${LABSOUND_ROOT}/examples/src/ExamplesCommon.h"
    "${LABSOUND_ROOT}/examples/src/Examples.hpp"
    "${LABSOUND_ROOT}/examples/src/ExamplesMain.cpp")

add_executable(LabSoundExample  ${labsound_examples_src})

_set_cxx_14(LabSoundExample)
_set_compile_options(LabSoundExample)

if (APPLE)
#    if(CMAKE_OSX_SYSROOT MATCHES ".*iphoneos.*")
#        set(DARWIN_LIBS
#            "-framework AudioToolbox"
#            "-framework Accelerate"
#            "-framework CoreAudio")
#    else()
        set(DARWIN_LIBS
            "-framework AudioToolbox"
            "-framework AudioUnit"
            "-framework Accelerate"
            "-framework CoreAudio"
            "-framework Cocoa")
#    endif()
endif()

target_link_libraries(LabSoundExample LabSound ${DARWIN_LIBS})

set_target_properties(LabSoundExample PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY bin)

set_property(TARGET LabSoundExample PROPERTY FOLDER "examples")

install(TARGETS LabSoundExample
    BUNDLE DESTINATION bin
    RUNTIME DESTINATION bin)
