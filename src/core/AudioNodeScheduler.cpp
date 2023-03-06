//
// AudioNodeScheduler.cpp
// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeScheduler.h"

//#define ASN_PRINT(...)
#define ASN_PRINT(a) printf("Scheduler: %s", (a))

namespace lab {

const char * schedulingStateName(SchedulingState s)
{
    switch (s) {
        case SchedulingState::UNSCHEDULED: return "UNSCHEDULED";
        case SchedulingState::SCHEDULED:   return "SCHEDULED";
        case SchedulingState::FADE_IN:     return "FADE_IN";
        case SchedulingState::PLAYING:     return "PLAYING";
        case SchedulingState::STOPPING:    return "STOPPING";
        case SchedulingState::RESETTING:   return "RESETTING";
        case SchedulingState::FINISHING:   return "FINISHING";
        case SchedulingState::FINISHED:    return "FINISHED";
    }
    return "Unknown";
}

bool AudioNodeScheduler2::update(ContextRenderLock& r, int epoch_length)
{
    uint64_t epoch = r.context()->currentSampleFrame();
    _renderOffset = 0;
    _renderLength = epoch_length;
    
    while (!op.empty()) {
        auto top = op.top();
        if (top.epoch > epoch)
            break;
        
        if (top.state == SchedulingState::FADE_IN) {
            if (_playbackState < SchedulingState::FADE_IN) {
                // not playing, start, stop processing commands
                _playbackState = SchedulingState::FADE_IN;
                op.pop();
                break;
            }
            else if (_playbackState < SchedulingState::STOPPING) {
                // playing, do nothing, keep processing
                op.pop();
            }
            else {
                // in a stopped state, start over, stop processing commands
                _playbackState = SchedulingState::FADE_IN;
                op.pop();
                break;
            }
        }
        else if (top.state == SchedulingState::STOPPING) {
            if (_playbackState < SchedulingState::FADE_IN) {
                // not started, do nothing, keep processing commands
                op.pop();
            }
            else if (_playbackState < SchedulingState::STOPPING) {
                // stop, stop processing commands
                _playbackState = SchedulingState::STOPPING;
                op.pop();
                break;
            }
            else {
                // not started, nothing to do, keep processing commands
                op.pop();
            }
        }
    }
    
    switch (_playbackState) {
        case SchedulingState::UNSCHEDULED:
            break;
            
        case SchedulingState::SCHEDULED:
            // exactly on start, or late, get going straight away
            _renderOffset = 0;
            _renderLength = epoch_length;
            _playbackState = SchedulingState::FADE_IN;
            ASN_PRINT("fade in\n");
            break;
            
        case SchedulingState::FADE_IN:
            // start time has been achieved, there'll be one quantum with fade in applied.
            _renderOffset = 0;
            _playbackState = SchedulingState::PLAYING;
            ASN_PRINT("playing\n");
            // fall through to PLAYING to allow render length to be adjusted if stop-start is less than one quantum length
            
        case SchedulingState::PLAYING:
            break;
            
        case SchedulingState::STOPPING:
            _playbackState = SchedulingState::UNSCHEDULED;
            if (_onEnded)
                r.context()->enqueueEvent(_onEnded);
            ASN_PRINT("unscheduled\n");
            break;
            
        case SchedulingState::RESETTING:
        case SchedulingState::FINISHING:
        case SchedulingState::FINISHED:
            break;
    }
    return true;
}

} // lab
