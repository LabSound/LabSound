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
    ctx->setDeviceNode(std::make_shared<lab::AudioHardwareDeviceNode>(ctx.get(), outputConfig, inputConfig));
    ctx->lazyInitialize();
    return ctx;
}

std::unique_ptr<lab::AudioContext> MakeOfflineAudioContext(const AudioStreamConfig offlineConfig, float recordTimeMilliseconds)
{
    LOG("MakeOfflineAudioContext()");

    const float secondsToRun = (float) recordTimeMilliseconds * 0.001f;

    std::unique_ptr<AudioContext> ctx(new lab::AudioContext(true));
    ctx->setDeviceNode(std::make_shared<lab::NullDeviceNode>(ctx.get(), offlineConfig, secondsToRun));
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
            std::shared_ptr<AudioHardwareInputNode> inputNode(new AudioHardwareInputNode(hardwareDevice->AudioHardwareInputProvider()));
            return inputNode;
        }
        else
        {
            throw std::runtime_error("Cannot create AudioHardwareInputNode. Context does not own an AudioHardwareInputNode.");
        }
    }
    return {};
}

namespace
{
    char const * const NodeNames[] = {
        "ADSR",
        "Analyser",
        //            "AudioBasicProcessor",
        //            "AudioHardwareSource",
        "BiquadFilter",
        "ChannelMerger",
        "ChannelSplitter",
        "Clip",
        "Convolver",
        "Delay",
        "Diode",
        "DynamicsCompressor",
        //            "Function",
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
