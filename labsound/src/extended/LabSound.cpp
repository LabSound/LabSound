// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/DefaultAudioDestinationNode.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"
#include "LabSound/extended/LabSound.h"

#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>
#include <mutex>
#include <memory>

namespace lab
{

    std::shared_ptr<AudioHardwareSourceNode> MakeHardwareSourceNode(ContextRenderLock & r)
    {
        AudioSourceProvider * provider = r.context()->destination()->localAudioInputProvider();
        std::shared_ptr<AudioHardwareSourceNode> inputNode(new AudioHardwareSourceNode(r.context()->sampleRate(), provider));
        inputNode->setFormat(r, 1, r.context()->sampleRate());
        return inputNode;
    }

    std::unique_ptr<lab::AudioContext> MakeRealtimeAudioContext()
    {
        LOG("Initialize Realtime Context");
        std::unique_ptr<AudioContext> ctx(new lab::AudioContext(false));
        ctx->setDestinationNode(std::make_shared<lab::DefaultAudioDestinationNode>(ctx.get(), 44100));
        ctx->lazyInitialize();
        return ctx;
    }

    std::unique_ptr<lab::AudioContext> MakeOfflineAudioContext(float recordTimeMilliseconds)
    {
        LOG("Initialize Offline Context");

        // @tofix - hardcoded parameters
        const float sampleRate = 44100;
        float secondsToRun = (float) recordTimeMilliseconds * 0.001f;

        std::unique_ptr<AudioContext> ctx(new lab::AudioContext(true));
        ctx->setDestinationNode(std::make_shared<lab::OfflineAudioDestinationNode>(ctx.get(), sampleRate, secondsToRun, 2));
        ctx->lazyInitialize();
        return ctx;
    }

    std::unique_ptr<lab::AudioContext> MakeOfflineAudioContext(int numChannels, float recordTimeMilliseconds, float sampleRate)
    {
        LOG("Initialize Offline Context");

        std::unique_ptr<AudioContext> ctx(new lab::AudioContext(true));
        float secondsToRun = (float)recordTimeMilliseconds * 0.001f;
        ctx->setDestinationNode(std::make_shared<lab::OfflineAudioDestinationNode>(ctx.get(), sampleRate, secondsToRun, numChannels));
        ctx->lazyInitialize();
        return ctx;
    }
}

///////////////////////
// Logging Utilities //
///////////////////////

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

void LabSoundLog(const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char tmp[256] = { 0 };
    sprintf(tmp, "[%s @ %i]\n\t%s\n", file, line, fmt);
    vprintf(tmp, args);

    va_end(args);
}

void LabSoundAssertLog(const char* file, int line, const char * function, const char * assertion)
{
    if (assertion) printf("[%s @ %i] ASSERTION FAILED: %s\n", file, line, assertion);
    else printf("[%s @ %i] SHOULD NEVER BE REACHED\n", file, line);
}
