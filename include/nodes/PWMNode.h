
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

// PWMNode implements a comparison based PWM. That could be improved.

#pragma once

#include "AudioBasicProcessorNode.h"
#include "AudioParam.h"

namespace LabSound {

    // Expects two inputs.
    // input 0 is the carrier, and input 1 is the modulator.
    // If there is no modulator, then the node is a pass-through.

    class PWMNode : public WebCore::AudioBasicProcessorNode
    {
    public:
        static WTF::PassRefPtr<PWMNode> create(WebCore::AudioContext* context, float sampleRate)
        {
            return adoptRef(new PWMNode(context, sampleRate));
        }

    private:
        PWMNode(WebCore::AudioContext*, float sampleRate);
        virtual ~PWMNode();

        class PWMNodeInternal;
        PWMNodeInternal* data;
    };
    
}
