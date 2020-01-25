
# LabSound
# Will create a target named LabSound

set(labsnd_core_h
"${LABSOUND_ROOT}/include/LabSound/core/AnalyserNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioArray.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioBasicInspectorNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioBasicProcessorNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioBus.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioChannel.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioContext.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioDestinationNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioHardwareSourceNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioIOCallback.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioListener.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioNodeInput.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioNodeOutput.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioParam.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioParamTimeline.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioProcessor.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioScheduledSourceNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioSetting.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioSourceProvider.h"
"${LABSOUND_ROOT}/include/LabSound/core/AudioSummingJunction.h"
"${LABSOUND_ROOT}/include/LabSound/core/BiquadFilterNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/ChannelMergerNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/ChannelSplitterNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/ConcurrentQueue.h"
"${LABSOUND_ROOT}/include/LabSound/core/Constants.h"
"${LABSOUND_ROOT}/include/LabSound/core/ConvolverNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/DefaultAudioDestinationNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/DelayNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/DynamicsCompressorNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/FloatPoint3D.h"
"${LABSOUND_ROOT}/include/LabSound/core/GainNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/Macros.h"
"${LABSOUND_ROOT}/include/LabSound/core/Mixing.h"
"${LABSOUND_ROOT}/include/LabSound/core/OfflineAudioDestinationNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/OscillatorNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/PannerNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/SampledAudioNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/StereoPannerNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/WaveShaperNode.h"
"${LABSOUND_ROOT}/include/LabSound/core/WaveTable.h"
"${LABSOUND_ROOT}/include/LabSound/core/WindowFunctions.h"
)

set(labsnd_extended_h
"${LABSOUND_ROOT}/include/LabSound/extended/ADSRNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/AudioContextLock.h"
"${LABSOUND_ROOT}/include/LabSound/extended/AudioFileReader.h"
"${LABSOUND_ROOT}/include/LabSound/extended/BPMDelay.h"
"${LABSOUND_ROOT}/include/LabSound/extended/ClipNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/DiodeNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/FunctionNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/Logging.h"
"${LABSOUND_ROOT}/include/LabSound/extended/NoiseNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/PdNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/PeakCompNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/PingPongDelayNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/PowerMonitorNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/PWMNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/RealtimeAnalyser.h"
"${LABSOUND_ROOT}/include/LabSound/extended/RecorderNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/SampledInstrumentNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/SfxrNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/SpatializationNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/SpectralMonitorNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/SupersawNode.h"
"${LABSOUND_ROOT}/include/LabSound/extended/Util.h"
)

set(labsnd_core
"${LABSOUND_ROOT}/src/core/_SoundPipe_FFT.cpp"
"${LABSOUND_ROOT}/src/core/AnalyserNode.cpp"
"${LABSOUND_ROOT}/src/core/AudioBasicInspectorNode.cpp"
"${LABSOUND_ROOT}/src/core/AudioBasicProcessorNode.cpp"
"${LABSOUND_ROOT}/src/core/AudioBus.cpp"
"${LABSOUND_ROOT}/src/core/AudioChannel.cpp"
"${LABSOUND_ROOT}/src/core/AudioContext.cpp"
"${LABSOUND_ROOT}/src/core/AudioDestinationNode.cpp"
"${LABSOUND_ROOT}/src/core/AudioHardwareSourceNode.cpp"
"${LABSOUND_ROOT}/src/core/AudioListener.cpp"
"${LABSOUND_ROOT}/src/core/AudioNode.cpp"
"${LABSOUND_ROOT}/src/core/AudioNodeInput.cpp"
"${LABSOUND_ROOT}/src/core/AudioNodeOutput.cpp"
"${LABSOUND_ROOT}/src/core/AudioParam.cpp"
"${LABSOUND_ROOT}/src/core/AudioParamTimeline.cpp"
"${LABSOUND_ROOT}/src/core/AudioScheduledSourceNode.cpp"
"${LABSOUND_ROOT}/src/core/AudioSummingJunction.cpp"
"${LABSOUND_ROOT}/src/core/BiquadFilterNode.cpp"
"${LABSOUND_ROOT}/src/core/ChannelMergerNode.cpp"
"${LABSOUND_ROOT}/src/core/ChannelSplitterNode.cpp"
"${LABSOUND_ROOT}/src/core/ConvolverNode.cpp"
"${LABSOUND_ROOT}/src/core/ConvolverNodeDeprecated.cpp"
"${LABSOUND_ROOT}/src/core/DefaultAudioDestinationNode.cpp"
"${LABSOUND_ROOT}/src/core/DelayNode.cpp"
"${LABSOUND_ROOT}/src/core/DynamicsCompressorNode.cpp"
"${LABSOUND_ROOT}/src/core/GainNode.cpp"
"${LABSOUND_ROOT}/src/core/OfflineAudioDestinationNode.cpp"
"${LABSOUND_ROOT}/src/core/OscillatorNode.cpp"
"${LABSOUND_ROOT}/src/core/PannerNode.cpp"
"${LABSOUND_ROOT}/src/core/RealtimeAnalyser.cpp"
"${LABSOUND_ROOT}/src/core/SampledAudioNode.cpp"
"${LABSOUND_ROOT}/src/core/StereoPannerNode.cpp"
"${LABSOUND_ROOT}/src/core/WaveShaperNode.cpp"
"${LABSOUND_ROOT}/src/core/WaveTable.cpp"
)

file(GLOB labsnd_extended   "${LABSOUND_ROOT}/src/extended/*")
file(GLOB labsnd_int_h      "${LABSOUND_ROOT}/src/internal/*")
file(GLOB labsnd_int_src    "${LABSOUND_ROOT}/src/internal/src/*")

# backend selection

if (IOS)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" ON)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" OFF)
elseif (APPLE)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" OFF)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" ON)
elseif (WIN32)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" OFF)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" ON)
elseif (UNIX)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" OFF)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" ON)
elseif (ANDROID)
    option(LABSOUND_USE_MINIAUDIO "Use miniaudio" ON)
    option(LABSOUND_USE_RTAUDIO "Use RtAudio" OFF)
else ()
    message(FATAL, " Untested platform. Please try miniaudio and report results on the LabSound issues page")
endif()

if (LABSOUND_USE_MINIAUDIO AND LABSOUND_USE_RTAUDIO)
    message(FATAL, " Specify only one backend")
elseif(NOT LABSOUND_USE_MINIAUDIO AND NOT LABSOUND_USE_RTAUDIO)
    message(FATAL, " Specify at least one backend")
endif()

if (LABSOUND_USE_MINIAUDIO)
    message(INFO, "Using miniaudio backend")
    set(labsnd_backend
        "${LABSOUND_ROOT}/src/backends/miniaudio/AudioDestinationMiniaudio.cpp"
        "${LABSOUND_ROOT}/src/backends/miniaudio/AudioDestinationMiniaudio.h"
        "${LABSOUND_ROOT}/src/backends/miniaudio/miniaudio.h"
    )
elseif (LABSOUND_USE_RTAUDIO)
    message(INFO, "Using RtAudio backend")
    set(labsnd_backend
        "${LABSOUND_ROOT}/src/backends/RtAudio/AudioDevice_RtAudio.cpp"
        "${LABSOUND_ROOT}/src/backends/RtAudio/AudioDevice_RtAudio.h"
        "${LABSOUND_ROOT}/src/backends/RtAudio/RtAudio.cpp"
        "${LABSOUND_ROOT}/src/backends/RtAudio/RtAudio.h"
    )
endif()


# FFT

if (IOS)
    set(labsnd_fft_src "${LABSOUND_ROOT}/src/backends/darwin/FFTFrameDarwin.cpp")
elseif (APPLE)
    set(labsnd_fft_src "${LABSOUND_ROOT}/src/backends/darwin/FFTFrameDarwin.cpp")
elseif (WIN32)
    file(GLOB labsnd_fft_src "${LABSOUND_ROOT}/third_party/kissfft/src/*")
elseif (UNIX)
    file(GLOB labsnd_fft_src "${LABSOUND_ROOT}/third_party/kissfft/src/*")
endif()

# TODO ooura or kissfft? benchmark and choose. Then benchmark vs FFTFrameDarwin
set(ooura_src
    "${LABSOUND_ROOT}/third_party/ooura/src/fftsg.cpp"
    "${LABSOUND_ROOT}/third_party/ooura/fftsg.h")

add_library(LabSound STATIC
    "${LABSOUND_ROOT}/include/LabSound/LabSound.h"
    ${labsnd_core_h}     ${labsnd_core}
    ${labsnd_extended_h} ${labsnd_extended}
    ${labsnd_int_h}      ${labsnd_int_src}
    ${labsnd_backend}
    ${labsnd_fft_src}
    ${ooura_src}
 )

if (APPLE)
    set_target_properties(LabSound PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER com.Lab.LabSound
        VERSION 0.13.0
        SOVERSION 1.0.0
        PUBLIC_HEADER "${labsnd_core_h} ${labsnd_extended_h}"
    )
    set_property(SOURCE ${labsnd_core_h}
        PROPERTY MACOSX_PACKAGE_LOCATION Headers/LabSound/core)
    set_property(SOURCE ${labsnd_extended_h}
        PROPERTY MACOSX_PACKAGE_LOCATION Headers/LabSound/extended)
endif()

_set_cxx_14(LabSound)
_set_compile_options(LabSound)

target_include_directories(LabSound PUBLIC
    ${LABSOUND_ROOT}/include)

target_include_directories(LabSound PRIVATE
    ${LABSOUND_ROOT}/src
    ${LABSOUND_ROOT}/src/internal
    ${LABSOUND_ROOT}/third_party
    ${LABSOUND_ROOT}/third_party/libnyquist/include)

if (MSVC_IDE)
    # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
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
elseif (APPLE)
    target_compile_definitions(LabSound PRIVATE __MACOSX_CORE__=1)
else()
    if (LABSOUND_JACK)
        target_compile_definitions(LabSound PRIVATE __UNIX_JACK__=1)
        set(LIBNYQUIST_JACK ON)
    elseif (LABSOUND_PULSE)
        target_compile_definitions(LabSound PRIVATE __LINUX_PULSE__=1)
        set(LIBNYQUIST_PULSE ON)
    elseif (LABSOUND_ASOUND)
        target_compile_definitions(LabSound PRIVATE __LINUX_ALSA__=1)
        set(LIBNYQUIST_ASOUND ON)
    else()
        message(FATAL, "On Linux, one of LABSOUND_JACK, LABSOUND_PULSE, or LABSOUND_ASOUND must be set ON.")
    endif()
endif()

target_link_libraries(LabSound libnyquist libopus libwavpack)

install(TARGETS LabSound
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    FRAMEWORK DESTINATION lib
    RUNTIME DESTINATION bin)

install(FILES "${LABSOUND_ROOT}/include/LabSound/LabSound.h"
    DESTINATION include/LabSound)
install(FILES ${labsnd_core_h}
    DESTINATION include/LabSound/core)
install(FILES ${labsnd_extended_h}
    DESTINATION include/LabSound/extended)
install(DIRECTORY
    assets/hrtf
    assets/images
    assets/impulse
    assets/json
    assets/pd
    assets/samples
    DESTINATION share/LabSound)

source_group(include FILES "${LABSOUND_ROOT}/include/LabSound")
source_group(include\\core FILES ${labsnd_core_h})
source_group(include\\extended FILES ${labsnd_extended_h})
source_group(src\\backends FILES ${labsnd_backend})
source_group(src\\core FILES ${labsnd_core})
source_group(src\\extended FILES ${labsnd_extended})
source_group(src\\internal FILES ${labsnd_int_h})
source_group(src\\internal\\src FILES ${labsnd_int_src})
source_group(third_party\\kissfft FILES ${third_kissfft})
source_group(third_party\\ooura FILES ${ooura_src})
source_group(third_party\\rtaudio FILES ${third_rtaudio})

configure_file("${LABSOUND_ROOT}/cmake/LabSoundConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/LabSoundConfig.cmake" @ONLY)
install(FILES
  "${PROJECT_BINARY_DIR}/LabSoundConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

add_library(Lab::Sound ALIAS LabSound)
