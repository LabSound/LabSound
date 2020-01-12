// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/LabSound.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/DefaultAudioDestinationNode.h"

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
namespace Sound
{

    std::shared_ptr<AudioHardwareSourceNode> MakeHardwareSourceNode(ContextRenderLock & r)
    {
        AudioSourceProvider * provider = r.context()->destination()->localAudioInputProvider();
        std::shared_ptr<AudioHardwareSourceNode> inputNode(new AudioHardwareSourceNode(r.context()->sampleRate(), provider));
        inputNode->setFormat(r, 1, r.context()->sampleRate());
        return inputNode;
    }

    std::unique_ptr<lab::AudioContext> MakeRealtimeAudioContext(uint32_t numChannels, float sample_rate)
    {
        LOG("Initialize Realtime Context");
        std::unique_ptr<AudioContext> ctx(new lab::AudioContext(false));
        ctx->setDestinationNode(std::make_shared<lab::DefaultAudioDestinationNode>(ctx.get(), numChannels, sample_rate));
        ctx->lazyInitialize();
        return ctx;
    }

    std::unique_ptr<lab::AudioContext> MakeOfflineAudioContext(uint32_t numChannels, float recordTimeMilliseconds)
    {
        LOG("Initialize Offline Context");

        float secondsToRun = (float) recordTimeMilliseconds * 0.001f;

        std::unique_ptr<AudioContext> ctx(new lab::AudioContext(true));
        ctx->setDestinationNode(std::make_shared<lab::OfflineAudioDestinationNode>(ctx.get(), LABSOUND_DEFAULT_SAMPLERATE, secondsToRun, numChannels));
        ctx->lazyInitialize();
        return ctx;
    }

    std::unique_ptr<lab::AudioContext> MakeOfflineAudioContext(uint32_t numChannels, float recordTimeMilliseconds, float sampleRate)
    {
        LOG("Initialize Offline Context");

        std::unique_ptr<AudioContext> ctx(new lab::AudioContext(true));
        float secondsToRun = (float) recordTimeMilliseconds * 0.001f;
        ctx->setDestinationNode(std::make_shared<lab::OfflineAudioDestinationNode>(ctx.get(), sampleRate, secondsToRun, numChannels));
        ctx->lazyInitialize();
        return ctx;
    }

    namespace
    {
        char const * const NodeNames[] = {
            "ADSR",
            "Analyser",
            "AudioBasicProcessor",
            "AudioHardwareSource",
            "BiquadFilter",
            "ChannelMerger",
            "ChannelSplitter",
            "Clip",
            "Convolver",
            "Delay",
            "Diode",
            "DynamicsCompressor",
            "Function",
            "Gain",
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

}  // Sound
}  // lab

///////////////////////
// Logging Utilities //
///////////////////////

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

void LabSoundLog(const char * file, int line, const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char tmp[256] = {0};
    sprintf(tmp, "[%s @ %i]\n\t%s\n", file, line, fmt);
    vprintf(tmp, args);

    va_end(args);
}

void LabSoundAssertLog(const char * file_, int line, const char * function_, const char * assertion_)
{
    const char * file = file_ ? file_ : "Unknown source file";
    const char * function = function_ ? function_ : "";
    const char * assertion = assertion_ ? assertion_ : "Assertion failed";
    printf("Assertion: %s:%s:%d - %s\n", function, file_, line, assertion);
}
