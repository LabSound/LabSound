// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#ifndef ADSR_NODE_H
#define ADSR_NODE_H

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioSetting.h"

namespace lab
{

/// @TODO ADSRNode should be a scheduled node, not only a basic processor node.
/// LabSound avoids multiple inheritance, so this requires a bit of thought.

class ADSRNode : public AudioBasicProcessorNode
{
    class ADSRNodeImpl;
    ADSRNodeImpl * adsr_impl;

public:
    ADSRNode(AudioContext &);
    virtual ~ADSRNode();

    virtual const char* name() const override { return "ADSR"; }

    // noteOn() will start applying an envelope to the incoming signal. The node will
    // progress through attack_time, ramping to attack_level. decay_time will settle the
    // the amplitude at sustain_level and continue indefinitely. Using the default |when|
    // value of 0.0, the envelope will begin immediately, otherwise delayed until
    // |when| seconds in the future. If sustain_level is 0, then noteOn produces a short
    // envelope between (attack_time + decay_time) without using any of the remaining 
    // stages via noteOff(). Calling noteOn() many times is OK, but if a current
    // envelope has not finished, the behavior is to quickly decay to zero and begin
    // the new attack. 
    void noteOn(const double when = 0.0);

    // If started using noteOn(), noteOff()` will progress the envelope from sustain_level
    // to 0.0 over the release_time period. Using the default |when| value of 0.0, the envelope 
    // will end immediately, otherwise delayed until |when| seconds in the future. 
    void noteOff(const double when = 0.0);

    // ADSR bang() will issue a noteOn/noteOff pair. If a length value is given, it will start
    // immediately and finish after (now + length). If length is the default value (0.f), then
    // bang will issue an impulse, a single 1.0 sample at the beginning of the render quanta.
    virtual void bang(const double length = 0.0);

    // This function will return true after the release period (only if a noteOff has been issued). 
    bool finished(ContextRenderLock &);

    void set(float attack_time, float attack_level, float decay_time, float sustain_level, float release_time);

    std::shared_ptr<AudioSetting> attackTime() const;  // Duration in seconds
    std::shared_ptr<AudioSetting> attackLevel() const;  // Level
    std::shared_ptr<AudioSetting> decayTime() const;  // Duration in seconds
    std::shared_ptr<AudioSetting> sustainLevel() const;  // Level
    std::shared_ptr<AudioSetting> releaseTime() const;  // Duration in seconds
};

}

#endif
