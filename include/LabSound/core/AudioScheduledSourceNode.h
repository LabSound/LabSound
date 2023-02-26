// License: BSD 3 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioScheduledSourceNode_h
#define AudioScheduledSourceNode_h

#include "LabSound/core/AudioNode.h"

namespace lab
{

/*
    AudioScheduledSourceNode adds a scheduling interface to
    AudioNode. The scheduler is in the base class, but only nodes
    derived from AudioScheduledSourceNode can be scheduled.
 */

class AudioScheduledSourceNode : public AudioNode
{
public:
    AudioScheduledSourceNode() = delete;

    explicit AudioScheduledSourceNode(AudioContext & ac, AudioNodeDescriptor const & desc) : AudioNode(ac, desc) { }
    virtual ~AudioScheduledSourceNode() = default;

    virtual bool isScheduledNode() const override { return true; }

    bool isPlayingOrScheduled() const
    {
        return _self->scheduler._playbackState >= SchedulingState::SCHEDULED &&
               _self->scheduler._playbackState <= SchedulingState::STOPPING;
    }

    // Start time, measured as seconds from the current epochal time
    void start(float when) { _self->scheduler.start(when); }

    uint64_t startWhen() const { return _self->scheduler._startWhen; }

    // Stop time, measured as seconds from the current epochal time
    void stop(float when) { _self->scheduler.stop(when); }

    SchedulingState playbackState() const { return _self->scheduler._playbackState; }
    bool hasFinished() const { return _self->scheduler.hasFinished(); }
    void setOnEnded(std::function<void()> fn) { _self->scheduler._onEnded = fn; }
};

}  // namespace lab

#endif  // AudioScheduledSourceNode_h
