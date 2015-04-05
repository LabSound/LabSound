// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound.h"
#include "AudioContext.h"
#include "AudioContextLock.h"
#include "ExceptionCodes.h"
#include "DefaultAudioDestinationNode.h"

#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>

namespace LabSound
{

    std::timed_mutex g_TimedMutex;
    std::thread g_GraphUpdateThread;

    std::shared_ptr<LabSound::AudioContext> mainContext;
    
    const int update_rate_ms = 10;

    void UpdateGraph()
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
    
    std::shared_ptr<LabSound::AudioContext> init() 
	{
        LOG("Initialize Context");
        
        // Create an audio context object with the default audio destination
        mainContext = std::make_shared<LabSound::AudioContext>();
        mainContext->setDestinationNode(std::make_shared<DefaultAudioDestinationNode>(mainContext));
        mainContext->initHRTFDatabase();
        mainContext->lazyInitialize();

        g_GraphUpdateThread = std::thread(UpdateGraph);

        return mainContext;
    }


    void finish(std::shared_ptr<LabSound::AudioContext> context) 
	{
        LOG("Finish Context");

		// Invalidate local shared_ptr
        mainContext.reset();

		// Join update thread
		if (g_GraphUpdateThread.joinable()) g_GraphUpdateThread.join();
        
        for (int i = 0; i < 4; ++i) 
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

} // LabSound

