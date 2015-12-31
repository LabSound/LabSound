// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ADSR_NODE_H
#define ADSR_NODE_H

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/GainNode.h"

namespace lab
{

    class ADSRNode : public AudioBasicProcessorNode
    {
        class ADSRNodeInternal;
        ADSRNodeInternal * internalNode;
        
    public:
        
        ADSRNode(float sampleRate);
        virtual ~ADSRNode();
        
        // If noteOn is called before noteOff has finished, a pop can occur. Polling
        // finished and avoiding noteOn while finished is true can avoid the popping.
        void noteOn(double when);
        void noteOff(ContextRenderLock&, double when);
        
        bool finished(ContextRenderLock&); // if a noteOff has been issued, finished will be true after the release period

        void set(float aT, float aL, float d, float s, float r);

        std::shared_ptr<AudioParam> attackTime() const; // Duration in ms
        std::shared_ptr<AudioParam> attackLevel() const; // Duration in ms
        std::shared_ptr<AudioParam> decayTime() const; // Duration in ms
        std::shared_ptr<AudioParam> sustainLevel() const; // Level 0-10
        std::shared_ptr<AudioParam> releaseTime() const; // Duration in ms
    };
    
}

#endif
