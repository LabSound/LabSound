// Copyright (c) 2003-2015 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

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

namespace LabSound
{
    
    std::timed_mutex g_TimedMutex;
    std::thread g_GraphUpdateThread;
    
    std::shared_ptr<WebCore::AudioContext> mainContext;
    
    const int update_rate_ms = 10;

    static void UpdateGraph()
	{
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(update_rate_ms));
            if (mainContext)
            {
                ContextGraphLock g(mainContext, "LabSound::GraphUpdateThread");
                // test both because the mainContext might have been destructed during the acquisition of the main context,
                // particularly during app shutdown. No point in continuing to process.
                if (g.context() && mainContext)
                    g.context()->update(g);
            }
            else
            {
                break;
            }
        }
        LOG("LabSound GraphUpdateThread thread finished");
    }
    
    std::shared_ptr<WebCore::AudioContext> init()
    {
        LOG("Initialize Context");
        
        mainContext = std::make_shared<WebCore::AudioContext>();
        mainContext->setDestinationNode(std::make_shared<WebCore::DefaultAudioDestinationNode>(mainContext));
        mainContext->initHRTFDatabase();
        mainContext->lazyInitialize();
        
        g_GraphUpdateThread = std::thread(UpdateGraph);
        
        return mainContext;
    }
    
    std::shared_ptr<WebCore::AudioContext> initOffline(int millisecondsToRun)
    {
        LOG("Initialize Offline Context");
        
        const int sampleRate = 44100;
        
        auto framesPerMillisecond = sampleRate / 1000;
        auto totalFramesToRecord = millisecondsToRun * framesPerMillisecond;
        
        mainContext = std::make_shared<WebCore::AudioContext>(2, totalFramesToRecord, sampleRate);
        auto renderTarget = mainContext->getOfflineRenderTarget();
        mainContext->setDestinationNode(std::make_shared<WebCore::OfflineAudioDestinationNode>(mainContext, renderTarget.get()));
        mainContext->initHRTFDatabase();
        mainContext->lazyInitialize();
        
        g_GraphUpdateThread = std::thread(UpdateGraph);
        
        return mainContext;
    }
    
    void finish(std::shared_ptr<WebCore::AudioContext> context)
    {
        LOG("Finish Context");
        
        // Invalidate local shared_ptr
        mainContext.reset();
        
        // Join update thread
        if (g_GraphUpdateThread.joinable())
            g_GraphUpdateThread.join();
        
        for (int i = 0; i < 8; ++i)
        {
            ContextGraphLock g(context, "LabSound::finish");
            
            if (!g.context())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            else
            {
                // Stop now calls deleteMarkedNodes() and uninitialize()
                context->stop(g);
                return;
            }
        }
        
        LOG("Could not acquire lock for shutdown");
    }
    
} // end namespace LabSound
