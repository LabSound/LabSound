// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/Macros.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioBus.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioUtilities.h"
#include "internal/Assertions.h"

#include <algorithm>

using namespace std;

namespace lab {

const double UnknownTime = -1;

AudioScheduledSourceNode::AudioScheduledSourceNode() : m_playbackState(UNSCHEDULED_STATE), m_startTime(0), m_endTime(UnknownTime)
{
}

void AudioScheduledSourceNode::updateSchedulingInfo(ContextRenderLock& r,
                                                    size_t quantumFrameSize,
                                                    AudioBus * outputBus,
                                                    size_t & quantumFrameOffset,
                                                    size_t & nonSilentFramesToProcess)
{
    if (!outputBus)
        return;

    if (quantumFrameSize != AudioNode::ProcessingSizeInFrames)
        return;

    AudioContext * context = r.context();

    if (!context)
        return;
    
    float sampleRate = r.context()->sampleRate();
    
    // quantumStartFrame     : Start frame of the current time quantum.
    // quantumEndFrame       : End frame of the current time quantum.
    // startFrame            : Start frame for this source.
    // endFrame              : End frame for this source.
    uint64_t quantumStartFrame = context->currentSampleFrame();
    uint64_t quantumEndFrame = quantumStartFrame + quantumFrameSize;
    uint64_t startFrame = AudioUtilities::timeToSampleFrame(m_startTime, sampleRate);
    uint64_t endFrame = m_endTime == UnknownTime ? 0 : AudioUtilities::timeToSampleFrame(m_endTime, sampleRate);

    // If end time is known and it's already passed, then don't do any more rendering
    if (m_endTime != UnknownTime && endFrame <= quantumStartFrame)
        finish(r);

    // If unscheduled, or finished, or out of time, output silence
    if (m_playbackState == UNSCHEDULED_STATE || m_playbackState == FINISHED_STATE || startFrame >= quantumEndFrame) 
    {
        // Output silence.
        outputBus->zero();
        nonSilentFramesToProcess = 0;
        return;
    }

    // Check if it's time to start playing.
    if (m_playbackState == SCHEDULED_STATE) 
    {
        m_playbackState = PLAYING_STATE;
    }

    quantumFrameOffset = startFrame > quantumStartFrame ? startFrame - quantumStartFrame : 0;
    quantumFrameOffset = std::min(quantumFrameOffset, quantumFrameSize); // clamp to valid range
    nonSilentFramesToProcess = quantumFrameSize - quantumFrameOffset;

    if (!nonSilentFramesToProcess)
    {
        // Output silence.
        outputBus->zero();
        return;
    }

    // Handle silence before we start playing.
    // Zero any initial frames representing silence leading up to a rendering start time in the middle of the quantum.
    if (quantumFrameOffset)
    {
        for (unsigned i = 0; i < outputBus->numberOfChannels(); ++i)
            memset(outputBus->channel(i)->mutableData(), 0, sizeof(float) * quantumFrameOffset);
    }

    // Handle silence after we're done playing.
    // If the end time is somewhere in the middle of this time quantum, then zero out the
    // frames from the end time to the very end of the quantum.
    if (m_endTime != UnknownTime && endFrame >= quantumStartFrame && endFrame < quantumEndFrame) {
        size_t zeroStartFrame = endFrame - quantumStartFrame;
        size_t framesToZero = quantumFrameSize - zeroStartFrame;

        bool isSafe = zeroStartFrame < quantumFrameSize && framesToZero <= quantumFrameSize && zeroStartFrame + framesToZero <= quantumFrameSize;
        ASSERT(isSafe);

        if (isSafe) {
            if (framesToZero > nonSilentFramesToProcess)
                nonSilentFramesToProcess = 0;
            else
                nonSilentFramesToProcess -= framesToZero;

            for (unsigned i = 0; i < outputBus->numberOfChannels(); ++i)
                memset(outputBus->channel(i)->mutableData() + zeroStartFrame, 0, sizeof(float) * framesToZero);
        }

        finish(r);
    }
}

void AudioScheduledSourceNode::start(double when)
{
    // https://github.com/modulesio/LabSound/pull/17
    // allow start to be scheduled independently of current state

    if (m_playbackState == PLAYING_STATE)
        return;

    if (!std::isfinite(when) || (when < 0)) {
        return;
    }

    m_startTime = when;
    m_playbackState = SCHEDULED_STATE;
}

void AudioScheduledSourceNode::stop(double when)
{
    // https://github.com/modulesio/LabSound/pull/17
    // allow stop to be scheduled independently of current state
    // original:
    // if (m_playbackState == FINISHED_STATE || m_playbackState == UNSCHEDULED_STATE))
    //    return;
    
    if (!std::isfinite(when))
        return;
    
    when = max(0.0, when);
    m_endTime = when;
}

void AudioScheduledSourceNode::reset(ContextRenderLock&) 
{
    m_playbackState = UNSCHEDULED_STATE;
    m_endTime = UnknownTime;
}

void AudioScheduledSourceNode::finish(ContextRenderLock& r)
{
    m_playbackState = FINISHED_STATE;
    r.context()->enqueueEvent(m_onEnded);
}

} // namespace lab
