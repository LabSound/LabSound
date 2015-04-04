// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound.h"
#include "AudioContext.h"
#include "AudioContextLock.h"
#include "ExceptionCodes.h"
#include "DefaultAudioDestinationNode.h"
#include "WTF/MainThread.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>

namespace LabSound {

    
    std::timed_mutex g_TimedMutex;
    std::thread g_GraphUpdateThread;

    std::shared_ptr<LabSound::AudioContext> mainContext;

	std::atomic<bool> runGraphUpdate = true;
    
    const int updateRate_ms = 10;

    void UpdateGraph() 
	{
        while (runGraphUpdate)
		{
            std::chrono::milliseconds sleepDuration(updateRate_ms);
            std::this_thread::sleep_for(sleepDuration);
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
                // thread is finished
                //break;
            }
        }
        printf("LabSound GraphUpdateThread thread finished\n");
    }
    
    std::shared_ptr<LabSound::AudioContext> init() 
	{
        // Initialize threads for the WTF library
       // WTF::initializeThreading();
       // WTF::initializeMainThread();
        
        // Create an audio context object with the default audio destination
        ExceptionCode ec;
        mainContext = LabSound::AudioContext::create(ec);
        mainContext->setDestinationNode(std::make_shared<DefaultAudioDestinationNode>(mainContext));
        mainContext->initHRTFDatabase();
        mainContext->lazyInitialize();

        g_GraphUpdateThread = std::thread(UpdateGraph);

		//g_GraphUpdateThread.detach();
        
        return mainContext;
    }


    void finish(std::shared_ptr<LabSound::AudioContext> context) 
	{

		std::cout << "Context Use Count " << context.use_count() << std::endl; 

		// Halt the graph update thread
		runGraphUpdate = false;

        mainContext.reset(); // -> -1 to the use_count

		std::cout << "Context Use Count " << context.use_count() << std::endl; 

        std::this_thread::sleep_for(std::chrono::milliseconds(updateRate_ms * 2));
        
        for (int i = 0; i < 10; ++i) 
		{
            ContextGraphLock g(context, "LabSound::finish");

            if (!g.context()) 
			{
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            else 
			{
                context->stop(g);
                context->deleteMarkedNodes();
                context->uninitialize(g);
                return;
            }
        }

        std::cerr << "LabSound could not acquire lock for shutdown" << std::endl;
    }

} // LabSound

