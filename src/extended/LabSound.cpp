// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/LabSound.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

namespace lab
{
const std::vector<AudioDeviceInfo> MakeAudioDeviceList()
{
    LOG("MakeAudioDeviceList()");
    return AudioDevice::MakeAudioDeviceList();
}

const uint32_t GetDefaultOutputAudioDeviceIndex()
{
    LOG("GetDefaultOutputAudioDeviceIndex()");
    return AudioDevice::GetDefaultOutputAudioDeviceIndex();
}

const uint32_t GetDefaultInputAudioDeviceIndex()
{
    LOG("GetDefaultInputAudioDeviceIndex()");
    return AudioDevice::GetDefaultInputAudioDeviceIndex();
}

std::unique_ptr<lab::AudioContext> MakeRealtimeAudioContext(const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig)
{
    LOG("MakeRealtimeAudioContext()");

    std::unique_ptr<AudioContext> ctx(new lab::AudioContext(false));
    ctx->setDeviceNode(std::make_shared<lab::AudioHardwareDeviceNode>(*ctx.get(), outputConfig, inputConfig));
    ctx->lazyInitialize();
    return ctx;
}

std::unique_ptr<lab::AudioContext> MakeOfflineAudioContext(const AudioStreamConfig offlineConfig, float recordTimeMilliseconds)
{
    LOG("MakeOfflineAudioContext()");

    const float secondsToRun = (float) recordTimeMilliseconds * 0.001f;

    std::unique_ptr<AudioContext> ctx(new lab::AudioContext(true));
    ctx->setDeviceNode(std::make_shared<lab::NullDeviceNode>(*ctx.get(), offlineConfig, secondsToRun));
    ctx->lazyInitialize();
    return ctx;
}

std::shared_ptr<AudioHardwareInputNode> MakeAudioHardwareInputNode(ContextRenderLock & r)
{
    LOG("MakeAudioHardwareInputNode()");

    auto device = r.context()->device();

    if (device)
    {
        if (auto * hardwareDevice = dynamic_cast<AudioHardwareDeviceNode *>(device.get()))
        {
            std::shared_ptr<AudioHardwareInputNode> inputNode( 
                new AudioHardwareInputNode(*r.context(), hardwareDevice->AudioHardwareInputProvider()));
            return inputNode;
        }
        else
        {
            throw std::runtime_error("Cannot create AudioHardwareInputNode. Context does not own an AudioHardwareInputNode.");
        }
    }
    return {};
}

AudioStreamConfig GetDefaultInputAudioDeviceConfiguration()
{
    AudioStreamConfig inputConfig;

    const std::vector<AudioDeviceInfo> audioDevices = lab::MakeAudioDeviceList();
    const uint32_t default_output_device = lab::GetDefaultOutputAudioDeviceIndex();
    const uint32_t default_input_device = lab::GetDefaultInputAudioDeviceIndex();

    AudioDeviceInfo defaultInputInfo;
    for (auto& info : audioDevices)
    {
        if (info.index == default_input_device) 
            defaultInputInfo = info;
    }

    if (defaultInputInfo.index == -1)
        throw std::invalid_argument("the default audio input device was requested but none were found");

    inputConfig.device_index = defaultInputInfo.index;
    inputConfig.desired_channels = std::min(uint32_t(1), defaultInputInfo.num_input_channels);
    inputConfig.desired_samplerate = defaultInputInfo.nominal_samplerate;
    return inputConfig;
}

AudioStreamConfig GetDefaultOutputAudioDeviceConfiguration()
{
    AudioStreamConfig outputConfig;

    const std::vector<AudioDeviceInfo> audioDevices = lab::MakeAudioDeviceList();
    const uint32_t default_output_device = lab::GetDefaultOutputAudioDeviceIndex();
    const uint32_t default_input_device = lab::GetDefaultInputAudioDeviceIndex();

    AudioDeviceInfo defaultOutputInfo;
    for (auto& info : audioDevices)
    {
        if (info.index == default_output_device) 
            defaultOutputInfo = info;
    }

    if (defaultOutputInfo.index == -1)
        throw std::invalid_argument("the default audio output device was requested but none were found");

    outputConfig.device_index = defaultOutputInfo.index;
    outputConfig.desired_channels = std::min(uint32_t(2), defaultOutputInfo.num_output_channels);
    outputConfig.desired_samplerate = defaultOutputInfo.nominal_samplerate;
    return outputConfig;
}

namespace
{
    char const * const NodeNames[] = {
        "ADSR",
        "Analyser",
        // "AudioBasicProcessor",
        // "AudioHardwareSource",
        "BiquadFilter",
        "ChannelMerger",
        "ChannelSplitter",
        "Clip",
        "Convolver",
        "Delay",
        "Diode",
        "DynamicsCompressor",
        // "Function",
        "Gain",
        "Granulation",
        "Noise",
        "Oscillator",
        "Panner",
#ifdef PD
        "PureData",
#endif
        "PeakCompressor",
        "PingPongDelay",
        "PolyBLEP",
        "PowerMonitor",
        "PWM",
        "Recorder",
        "SampledAudio",
        "Sfxr",
        "Spatialization",
        "SpectralMonitor",
        "StereoPanner",
        "SuperSaw",
        "WaveShaper", 
        nullptr};
}

char const * const * const AudioNodeNames()
{
    return NodeNames;
}

}  // lab

///////////////////////
// Logging Utilities //
///////////////////////

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

void LabSoundLog(const char * file, int line, const char * fmt, ...)
{
    printf("[%s @ %i]\n\t", file, line);
    va_list args;
    va_start(args, fmt);
    static std::vector<char> buff(1024);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void LabSoundAssertLog(const char * file_, int line, const char * function_, const char * assertion_)
{
    const char * file = file_ ? file_ : "Unknown source file";
    const char * function = function_ ? function_ : "";
    const char * assertion = assertion_ ? assertion_ : "Assertion failed";
    printf("Assertion: %s:%s:%d - %s\n", function, file_, line, assertion);
}
