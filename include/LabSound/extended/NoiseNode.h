// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef NOISE_NODE_H
#define NOISE_NODE_H

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"

namespace lab
{
class AudioSetting;

class NoiseNode : public AudioScheduledSourceNode
{

public:
    enum NoiseType
    {
        WHITE = 0,
        PINK = 1,
        BROWN = 2,
        _Count = 3
    };

    NoiseNode();
    virtual ~NoiseNode();

    virtual void process(ContextRenderLock &, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock &) override;

    NoiseType type() const;
    void setType(NoiseType newType);

private:
    virtual bool propagatesSilence(ContextRenderLock & r) const override;
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    std::shared_ptr<AudioSetting> _type;

    uint32_t whiteSeed = 1489853723;

    float lastBrown = 0;

    float pink0 = 0;
    float pink1 = 0;
    float pink2 = 0;
    float pink3 = 0;
    float pink4 = 0;
    float pink5 = 0;
    float pink6 = 0;
};
}

#endif
