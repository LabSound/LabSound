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
    std::unique_ptr<lab::AudioContext> MakeRealtimeAudioContext()
    {
        LOG("Initialize Realtime Context");
        std::unique_ptr<AudioContext> ctx(new lab::AudioContext());
        ctx->setDestinationNode(std::make_shared<lab::DefaultAudioDestinationNode>(ctx.get()));
        ctx->lazyInitialize();
        return ctx;
    }

    std::unique_ptr<lab::AudioContext> MakeOfflineAudioContext(const int millisecondsToRun)
    {
        LOG("Initialize Offline Context");

        // @tofix - hardcoded parameters
        const int sampleRate = 44100;
        const int framesPerMillisecond = sampleRate / 1000;
        const int totalFramesToRecord = millisecondsToRun * framesPerMillisecond;

        std::unique_ptr<AudioContext> ctx(new lab::AudioContext(2, totalFramesToRecord, sampleRate));
        auto renderTarget = ctx->getOfflineRenderTarget();
        ctx->setDestinationNode(std::make_shared<lab::OfflineAudioDestinationNode>(ctx.get(), renderTarget.get()));
        ctx->lazyInitialize();
        return ctx;
    }

    std::unique_ptr<lab::AudioContext> MakeOfflineAudioContext(int numChannels, size_t totalFramesToRecord, float sampleRate)
    {
        LOG("Initialize Offline Context");

        std::unique_ptr<AudioContext> ctx(new lab::AudioContext(numChannels, totalFramesToRecord, sampleRate));
        auto renderTarget = ctx->getOfflineRenderTarget();
        ctx->setDestinationNode(std::make_shared<lab::OfflineAudioDestinationNode>(ctx.get(), renderTarget.get()));
        ctx->lazyInitialize();
        return ctx;
    }

    void AcquireLocksForContext(const std::string id, AudioContext * ctx, std::function<void(ContextGraphLock & g, ContextRenderLock & r)> callback)
    {
        try
        {
            ContextGraphLock g(ctx, id);
            ContextRenderLock r(ctx, id);
            callback(g, r);
        }
        catch (const std::exception & e)
        {
            LOG_ERROR("caught exception %s", e.what());
        }
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
