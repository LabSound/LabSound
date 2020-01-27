// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/NullDeviceNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"

#include "internal/Assertions.h"

#include <algorithm>

using namespace std;
 
namespace lab {
    
const size_t renderQuantumSize = 128;    

NullDeviceNode::NullDeviceNode(AudioContext * context, const float sampleRate, const float lengthSeconds, const uint32_t numChannels) 
    : m_numChannels(numChannels), m_lengthSeconds(lengthSeconds), m_context(context)
{
    m_renderBus = std::unique_ptr<AudioBus>(new AudioBus(m_numChannels, renderQuantumSize));
}

NullDeviceNode::~NullDeviceNode()
{
    uninitialize();
}

void NullDeviceNode::initialize()
{
    if (isInitialized())
        return;

    AudioNode::initialize();
}

void NullDeviceNode::uninitialize()
{
    if (!isInitialized())
        return;

    if (m_renderThread.joinable())
    {
       m_renderThread.join();
    }

    AudioNode::uninitialize();
}

void NullDeviceNode::start()
{
    if (!m_startedRendering) 
    {
        m_startedRendering = true;

        m_renderThread = std::thread(&NullDeviceNode::offlineRender, this);
        
        // @tofix - ability to update main thread from here. Currently blocks until complete
        if (m_renderThread.joinable()) m_renderThread.join();

        if (m_context->offlineRenderCompleteCallback)
        {
            m_context->offlineRenderCompleteCallback();
        }

        m_startedRendering = false;
    }
    else
    {
        LOG("Offline rendering has already started");
    }
}

void NullDeviceNode::stop()
{

}

void NullDeviceNode::render(AudioBus * sourceBus, AudioBus * destinationBus, size_t numberOfFrames) 
{

}

uint64_t NullDeviceNode::currentSampleFrame() const 
{
    return {};
}

double NullDeviceNode::currentTime() const 
{
     return {};
}

double NullDeviceNode::currentSampleTime() const 
{
    return {};
}

void NullDeviceNode::offlineRender()
{
    LOG("Starting Offline Rendering");

    ASSERT(m_renderBus.get());
    if (!m_renderBus.get())
        return;

    bool isRenderBusAllocated = m_renderBus->length() >= renderQuantumSize;
    ASSERT(isRenderBusAllocated);
    if (!isRenderBusAllocated)
        return;
    
    bool isAudioContextInitialized = m_context->isInitialized();
    ASSERT(isAudioContextInitialized);
    if (!isAudioContextInitialized) 
        return;

    // Break up the render target into smaller "render quantize" sized pieces.
    size_t framesToProcess = static_cast<size_t>((m_lengthSeconds * m_context->sampleRate()) / renderQuantumSize);

    while (framesToProcess > 0)
    {
        render(0, m_renderBus.get(), renderQuantumSize);
        framesToProcess -= 1;
    }

    LOG("Stopping Offline Rendering");
}

} // namespace lab
