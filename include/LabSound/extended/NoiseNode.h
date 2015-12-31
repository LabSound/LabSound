// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef NOISE_NODE_H
#define NOISE_NODE_H

#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"

namespace lab 
{

    class NoiseNode : public AudioScheduledSourceNode 
    {

    public:

        enum NoiseType
        {
            WHITE,
            PINK,
            BROWN,
            NOISE_TYPE_END,
        };

        NoiseNode(float sampleRate);
        virtual ~NoiseNode();

        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;

        unsigned short type() const { return m_type; }

        void setType(NoiseType newType);

    private:

        virtual bool propagatesSilence(double now) const override;

        NoiseType m_type = WHITE;

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
