// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#ifndef ADSR_NODE_H
#define ADSR_NODE_H

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioSetting.h"

namespace lab
{

class ADSRNode : public AudioBasicProcessorNode
{
    class ADSRNodeInternal;
    ADSRNodeInternal * internalNode;

public:
    ADSRNode();
    virtual ~ADSRNode();

    // bang will attack, hold, decay, release
    virtual bool hasBang() const override { return true; }
    virtual void bang(ContextRenderLock &) override;

    // noteOn will attack, noteOff will decay, release
    // If noteOn is called before noteOff has finished, a pop can occur. Polling
    // finished and avoiding noteOn while finished is true can avoid the popping.
    void noteOn(double when);
    void noteOff(ContextRenderLock &, double when);

    bool finished(ContextRenderLock &);  // if a noteOff has been issued, finished will be true after the release period

    void set(float aT, float aL, float d, float s, float r);

    std::shared_ptr<AudioSetting> attackTime() const;  // Duration in seconds
    std::shared_ptr<AudioSetting> attackLevel() const;  // Level
    std::shared_ptr<AudioSetting> decayTime() const;  // Duration in seconds
    std::shared_ptr<AudioSetting> holdTime() const;  // Duration in seconds
    std::shared_ptr<AudioSetting> sustainLevel() const;  // Level
    std::shared_ptr<AudioSetting> releaseTime() const;  // Duration in seconds
};
}

#endif
