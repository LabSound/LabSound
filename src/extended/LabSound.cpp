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
    std::thread g_GraphUpdateThread;
    std::shared_ptr<lab::AudioContext> mainContext;
    const int update_rate_ms = 10;

    static void UpdateGraphThread()
    {
        LOG("Create UpdateGraphThread");
        while (true)
        {
            if (mainContext)
            {
                ContextGraphLock g(mainContext, "lab::UpdateGraphThread");
                if (mainContext && g.context())
                {
                    g.context()->update(g);
                }
            }
            else
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(update_rate_ms));
        }
        LOG("Destroy UpdateGraphThread");
    }

    std::shared_ptr<lab::AudioContext> MakeAudioContext()
    {
        LOG("Initialize Context");
        mainContext = std::make_shared<lab::AudioContext>();
        mainContext->setDestinationNode(std::make_shared<lab::DefaultAudioDestinationNode>(mainContext));

        mainContext->lazyInitialize();

        g_GraphUpdateThread = std::thread(UpdateGraphThread);

        return mainContext;
    }

    std::shared_ptr<lab::AudioContext> MakeOfflineAudioContext(const int millisecondsToRun)
    {
        LOG("Initialize Offline Context");

        // @tofix - hardcoded parameters
        const int sampleRate = 44100;
        const int framesPerMillisecond = sampleRate / 1000;
        const int totalFramesToRecord = millisecondsToRun * framesPerMillisecond;

        mainContext = std::make_shared<lab::AudioContext>(2, totalFramesToRecord, sampleRate);
        auto renderTarget = mainContext->getOfflineRenderTarget();
        mainContext->setDestinationNode(std::make_shared<lab::OfflineAudioDestinationNode>(mainContext, renderTarget.get()));

        mainContext->lazyInitialize();

        return mainContext;
    }

    std::shared_ptr<lab::AudioContext> MakeOfflineAudioContext(int numChannels, size_t totalFramesToRecord, float sampleRate)
    {
        LOG("Initialize Offline Context");

        mainContext = std::make_shared<lab::AudioContext>(numChannels, totalFramesToRecord, sampleRate);
        auto renderTarget = mainContext->getOfflineRenderTarget();
        mainContext->setDestinationNode(std::make_shared<lab::OfflineAudioDestinationNode>(mainContext, renderTarget.get()));

        mainContext->lazyInitialize();

        return mainContext;
    }

    void CleanupAudioContext(std::shared_ptr<lab::AudioContext> context)
    {
        LOG("Finish Context");

        // Invalidate local shared_ptr
        mainContext.reset();

        // Join update thread
        if (g_GraphUpdateThread.joinable())
            g_GraphUpdateThread.join();

        for (int i = 0; i < 8; ++i)
        {
            ContextGraphLock g(context, "lab::finish");

            if (!g.context())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            else
            {
                // Stop then calls deleteMarkedNodes() and uninitialize()
                context->stop(g);
                return;
            }
        }

        LOG("Could not acquire lock for shutdown");
    }

    void AcquireLocksForContext(const std::string id, std::shared_ptr<AudioContext> & ctx, std::function<void(ContextGraphLock & g, ContextRenderLock & r)> callback)
    {
        ContextGraphLock g(ctx, id);
        ContextRenderLock r(ctx, id);
        callback(g, r);
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
