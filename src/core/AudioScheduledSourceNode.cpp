// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/Macros.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"

#include <algorithm>

using namespace std;

namespace lab
{

AudioScheduledSourceNode::AudioScheduledSourceNode()
{
}

void AudioScheduledSourceNode::updateSchedulingInfo(ContextRenderLock & r, int quantumFrameSize, AudioBus * outputBus, int & quantumFrameOffset, int & nonSilentFramesToProcess)
{
    if (!outputBus)  return;
    if (quantumFrameSize != AudioNode::ProcessingSizeInFrames) return;
    AudioContext * context = r.context();
    if (!context) return;

    // not atomic, but will largely prevent the times from being updated
    // by another thread during the update calculations.
    if (m_pendingEndTime > UNKNOWN_TIME)
    {
        m_endTime = m_pendingEndTime;
        m_pendingEndTime = UNKNOWN_TIME;
    }
    if (m_pendingStartTime > UNKNOWN_TIME)
    {
        m_startTime = m_pendingStartTime;
        m_pendingStartTime = UNKNOWN_TIME;
    }

    const float sampleRate = r.context()->sampleRate();

    // quantumStartFrame     : Start frame of the current time quantum.
    // quantumEndFrame       : End frame of the current time quantum.
    // startFrame            : Start frame for this source.
    // endFrame              : End frame for this source.
    int64_t quantumStartFrame = context->currentSampleFrame();
    int64_t quantumEndFrame = quantumStartFrame + quantumFrameSize;
    int64_t startFrame = AudioUtilities::timeToSampleFrame(m_startTime, sampleRate);
    int64_t endFrame = m_endTime == UNKNOWN_TIME ? 0 : AudioUtilities::timeToSampleFrame(m_endTime, sampleRate);

    // If end time is known and it's already passed, then don't do any more rendering
    if (m_endTime != UNKNOWN_TIME && endFrame <= quantumStartFrame)
    {
        finish(r);
    }

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

    quantumFrameOffset = startFrame > quantumStartFrame ? static_cast<int>(startFrame - quantumStartFrame) : 0;
    quantumFrameOffset = quantumFrameOffset < quantumFrameSize ? quantumFrameOffset : quantumFrameSize;  // clamp to valid range
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
        for (int i = 0; i < outputBus->numberOfChannels(); ++i)
        {
            std::memset(outputBus->channel(i)->mutableData(), 0, sizeof(float) * quantumFrameOffset);
        }
    }

    // Handle silence after we're done playing.
    // If the end time is somewhere in the middle of this time quantum, then zero out the
    // frames from the end time to the very end of the quantum.
    if (m_endTime != UNKNOWN_TIME && endFrame >= quantumStartFrame && endFrame < quantumEndFrame)
    {
        int zeroStartFrame = static_cast<int>(endFrame - quantumStartFrame);
        int framesToZero = quantumFrameSize - zeroStartFrame;

        bool isSafe = zeroStartFrame < quantumFrameSize && framesToZero <= quantumFrameSize && zeroStartFrame + framesToZero <= quantumFrameSize;
        ASSERT(isSafe);

        if (isSafe)
        {
            if (framesToZero > nonSilentFramesToProcess) nonSilentFramesToProcess = 0;
            else nonSilentFramesToProcess -= framesToZero;

            for (int i = 0; i < outputBus->numberOfChannels(); ++i)
            {
                std::memset(outputBus->channel(i)->mutableData() + zeroStartFrame, 0, sizeof(float) * framesToZero);
            }
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

    if (!std::isfinite(when) || (when < 0))
    {
        return;
    }

    m_pendingStartTime = when;
    m_endTime = UNKNOWN_TIME;  // clear previous stop()s.
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
    m_pendingEndTime = when;
}

void AudioScheduledSourceNode::reset(ContextRenderLock &)
{
    m_pendingEndTime = UNKNOWN_TIME;
    m_playbackState = UNSCHEDULED_STATE;
}

void AudioScheduledSourceNode::finish(ContextRenderLock & r)
{
    m_playbackState = FINISHED_STATE;
    if (m_onEnded) r.context()->enqueueEvent(m_onEnded);
}

}  // namespace lab
