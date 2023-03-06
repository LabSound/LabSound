
// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AUDIONODESCHEDULER_H
#define AUDIONODESCHEDULER_H

#include "LabSound/extended/AudioContextLock.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <queue>

namespace lab {

enum class SchedulingState : int
{
    UNSCHEDULED = 0, // Initial playback state. Created, but not yet scheduled
    SCHEDULED,       // Scheduled to play (via noteOn() or noteGrainOn()), but not yet playing
    FADE_IN,         // First epoch, fade in, then play
    PLAYING,         // Generating sound
    STOPPING,        // Transitioning to finished
    RESETTING,       // Node is resetting to initial, unscheduled state
    FINISHING,       // Playing has finished
    FINISHED         // Node has finished
};
const char* schedulingStateName(SchedulingState);

class ContextRenderLock;

class AudioNodeScheduler2
{
public:
    struct ScheduleOp {
        SchedulingState state;  // the upcoming state
        uint64_t epoch;         // when that state occurs; this->_epoch is the clock
        friend bool operator< (ScheduleOp const& lhs, ScheduleOp const& rhs) {
            return rhs.epoch < lhs.epoch;
        }
    };

    explicit AudioNodeScheduler2() = default;
    ~AudioNodeScheduler2() = default;

    // Scheduling
    // when is absolute, e.g. the calculation to start in 0.5s might be
    //   when + (context.epoch + 0.5 * 44100)
    void start(uint64_t when) {
        ScheduleOp opFade { SchedulingState::FADE_IN, when };
        op.push(opFade);
    }
    void stop(uint64_t when) {
        ScheduleOp opFade { SchedulingState::STOPPING, when };
        op.push(opFade);
    }

    void reset() {
        std::priority_queue<ScheduleOp> clear;
        op.swap(clear);
        stop(0);
    }

    bool update(ContextRenderLock& r, int epoch_length);

    SchedulingState playbackState() const { return _playbackState; }

    // called by AudioNode when the sound is finished,
    // or noteOff/stop time has been reached
    void finish(ContextRenderLock& r) {
        if (_onEnded)
            r.context()->enqueueEvent(_onEnded);
    }
    
    bool hasFinished() const { return _playbackState == SchedulingState::FINISHED; }
    void setOnStart(std::function<void(double)> fn) { _onStart = fn; }
    void setOnEnded(std::function<void()> fn)       { _onEnded = fn; }

private:
    friend class AudioNode;
    int _renderOffset = 0;
    int _renderLength = 0;
    SchedulingState _playbackState = SchedulingState::UNSCHEDULED;
    std::priority_queue<ScheduleOp> op;  // upcoming events
    std::function<void()> _onEnded;
    std::function<void(double when)> _onStart;
};
} // namespace

#endif

