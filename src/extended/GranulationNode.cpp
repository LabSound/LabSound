// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/GranulationNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"

using namespace lab;

GranulationNode::GranulationNode() : AudioScheduledSourceNode(), m_sourceBus(std::make_shared<AudioSetting>("sourceBus", "SBUS", AudioSetting::Type::Bus))
{
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));
    initialize();
}

GranulationNode::~GranulationNode()
{
    uninitialize();
}

void GranulationNode::reset(ContextRenderLock&)
{

}

bool GranulationNode::RenderGranulation(ContextRenderLock & r, AudioBus * bus, size_t destinationFrameOffset, size_t numberOfFrames)
{
    if (!r.context())
        return false;

    // Sanity check destinationFrameOffset, numberOfFrames.
    size_t destinationLength = bus->length();

    bool isLengthGood = destinationLength <= 4096 && numberOfFrames <= 4096;
    ASSERT(isLengthGood);
    if (!isLengthGood)
        return false;

    bool isOffsetGood = destinationFrameOffset <= destinationLength && destinationFrameOffset + numberOfFrames <= destinationLength;
    ASSERT(isOffsetGood);
    if (!isOffsetGood)
        return false;

    // @fixme - use of shared_ptr in render/processing chain
    std::shared_ptr<AudioBus> srcBus = getBus();

    if (!bus || !srcBus)
        return false;

    // Offset the pointers to the correct offset frame.
    size_t writeIndex = destinationFrameOffset;

    size_t bufferLength = srcBus->length();
    double bufferSampleRate = srcBus->sampleRate();

    if (m_virtualReadIndex >= bufferLength) m_virtualReadIndex = 0; // reset to start

    double virtualEndFrame = static_cast<double>(bufferLength);
    double virtualDeltaFrames = virtualEndFrame;

    // Get local copy
    double virtualReadIndex = m_virtualReadIndex;

    // Render loop - reading from the source buffer to the destination using linear interpolation.
    int framesToProcess = static_cast<int>(numberOfFrames);

    {
        int readIndex = static_cast<int>(virtualReadIndex);
        int deltaFrames = static_cast<int>(virtualDeltaFrames);
        bufferLength = static_cast<int>(virtualEndFrame);

        while (framesToProcess > 0)
        {
            int framesToEnd = static_cast<int>(bufferLength) - readIndex;
            int framesThisTime = std::min(framesToProcess, framesToEnd);
            framesThisTime = std::max(0, framesThisTime);

            // Mono only
            std::memcpy(bus->channel(0)->mutableData() + writeIndex, srcBus->channel(0)->data() + readIndex, sizeof(float) * framesThisTime);

            writeIndex += framesThisTime;
            readIndex += framesThisTime;
            framesToProcess -= framesThisTime;
        }

        virtualReadIndex = readIndex;
    }

    bus->clearSilentFlag();

    m_virtualReadIndex = virtualReadIndex;

    return true;
}

bool GranulationNode::setGrainSource(ContextRenderLock & r, std::shared_ptr<AudioBus> buffer)
{
    ASSERT(m_sourceBus);
    m_sourceBus->setBus(buffer.get());
    output(0)->setNumberOfChannels(r, buffer ? buffer->numberOfChannels() : 0);
    m_virtualReadIndex = 0;
    return true;
}

void GranulationNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    AudioBus * outputBus = output(0)->bus(r);

    if (!isInitialized() || !outputBus->numberOfChannels()) 
    {
        outputBus->zero();
        return;
    }

    size_t quantumFrameOffset;
    size_t bufferFramesToProcess;
    updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, bufferFramesToProcess);

    if (!bufferFramesToProcess)
    {
        outputBus->zero();
        return;
    }

    if (!RenderGranulation(r, outputBus, quantumFrameOffset, bufferFramesToProcess))
    {
        outputBus->zero();
        return;
    }
    outputBus->clearSilentFlag();
}
    
bool GranulationNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}
    