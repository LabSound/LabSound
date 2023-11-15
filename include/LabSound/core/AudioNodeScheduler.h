
// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AUDIONODESCHEDULER_H
#define AUDIONODESCHEDULER_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>

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

class AudioNodeScheduler
{
public:
    explicit AudioNodeScheduler() = delete;
    explicit AudioNodeScheduler(float sampleRate);
    ~AudioNodeScheduler() = default;

    // Scheduling
    void start(double when);
    void stop(double when);

    // called when the sound is finished, or noteOff/stop time has been reached
    void finish(ContextRenderLock&);
    void reset();

    SchedulingState playbackState() const { return _playbackState; }
    bool hasFinished() const { return _playbackState == SchedulingState::FINISHED; }

    bool update(ContextRenderLock&, int epoch_length, const char* node_name);

    SchedulingState _playbackState = SchedulingState::UNSCHEDULED;

    // epoch is a long count at sample rate; 136 years at 48kHz

    std::atomic<uint64_t> _epoch{0}; // the epoch rendered currently in the busses
    uint64_t _epochLength = 0;        // number of frames in current epoch

    // start and stop epoch (absolute, not relative)
    uint64_t _startWhen = std::numeric_limits<uint64_t>::max();
    uint64_t _stopWhen = std::numeric_limits<uint64_t>::max();

    int _renderOffset = 0; // where rendering starts in the current frame
    int _renderLength = 0; // number of rendered frames in the current frame 

    float _sampleRate = 1;

    std::function<void()> _onEnded;

    std::function<void(double when)> _onStart;
};

} // namespace

#endif

