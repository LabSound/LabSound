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

namespace LabSound
{
	std::thread g_GraphUpdateThread;
	
	std::shared_ptr<WebCore::AudioContext> mainContext;
	
	const int update_rate_ms = 10;

	static void UpdateGraph()
	{
		LOG("Create GraphUpdateThread");
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(update_rate_ms));
			if (mainContext)
			{
				ContextGraphLock g(mainContext, "LabSound::GraphUpdateThread");
				if (mainContext && g.context())
				{
					g.context()->update(g);
				}
			}
			else
			{
				break;
			}
		}
		LOG("Destroy GraphUpdateThread");
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

} // LabSound
