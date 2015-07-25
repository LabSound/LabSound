/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AudioScheduledSourceNode_h
#define AudioScheduledSourceNode_h

#include "LabSound/core/AudioSourceNode.h"

namespace WebCore {

class AudioBus;

class AudioScheduledSourceNode : public AudioSourceNode 
{

public:

    // These are the possible states an AudioScheduledSourceNode can be in:
    // UNSCHEDULED_STATE - Initial playback state. Created, but not yet scheduled.
    // SCHEDULED_STATE - Scheduled to play (via noteOn() or noteGrainOn()), but not yet playing.
    // PLAYING_STATE - Generating sound.
    enum PlaybackState
	{
        UNSCHEDULED_STATE = 0,
        SCHEDULED_STATE = 1,
        PLAYING_STATE = 2,
        FINISHED_STATE = 3
    };
    
    AudioScheduledSourceNode(float sampleRate);
    virtual ~AudioScheduledSourceNode() {}

    // Scheduling.
    void start(double when);
    void stop(double when);
    
    double startTime() const { return m_startTime; }

    unsigned short playbackState() const { return static_cast<unsigned short>(m_playbackState); }

    bool isPlayingOrScheduled() const { return m_playbackState == PLAYING_STATE || m_playbackState == SCHEDULED_STATE; }

    bool hasFinished() const { return m_playbackState == FINISHED_STATE; }

	void reset() { m_playbackState = UNSCHEDULED_STATE; }

    // LabSound: If the node included ScheduledNode in its hierarchy, this will return true.
    // This is to save the cost of a dynamic_cast when scheduling nodes.
    virtual bool isScheduledNode() const override { return true; }

protected:

    // Get frame information for the current time quantum.
    // We handle the transition into PLAYING_STATE and FINISHED_STATE here,
    // zeroing out portions of the outputBus which are outside the range of startFrame and endFrame.
    //
    // Each frame time is relative to the context's currentSampleFrame().
    // quantumFrameOffset    : Offset frame in this time quantum to start rendering.
    // nonSilentFramesToProcess : Number of frames rendering non-silence (will be <= quantumFrameSize).
    void updateSchedulingInfo(ContextRenderLock &, size_t quantumFrameSize, AudioBus * outputBus, size_t & quantumFrameOffset, size_t & nonSilentFramesToProcess);

    // Called when we have no more sound to play or the noteOff/stop() time has been reached.
    void finish(ContextRenderLock&);

    PlaybackState m_playbackState;

    // m_startTime is the time to start playing based on the context's timeline (0 or a time less than the context's current time means "now").
    double m_startTime; // in seconds

    // m_endTime is the time to stop playing based on the context's timeline (0 or a time less than the context's current time means "now").
    // If it hasn't been set explicitly, then the sound will not stop playing (if looping) or will stop when the end of the AudioBuffer
    // has been reached.
    double m_endTime; // in seconds

    // this is the base declaration
    virtual void clearPannerNode() {}
};

} // namespace WebCore

#endif // AudioScheduledSourceNode_h
