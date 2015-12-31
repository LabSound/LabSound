// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"

#include "LabSound/extended/PWMNode.h"

#include "internal/AudioBus.h"

#include <iostream>

using namespace lab;

namespace lab {

    ////////////////////////////////////
    // Private PWMNode Implementation //
    ////////////////////////////////////

    class PWMNode::PWMNodeInternal : public lab::AudioProcessor
    {

    public:

        PWMNodeInternal(float sampleRate) : AudioProcessor(sampleRate, 2)
        {
        }

        virtual ~PWMNodeInternal() { }

        virtual void initialize() override { }

        virtual void uninitialize() override { }

        // Processes the source to destination bus.  The number of channels must match in source and destination.
        virtual void process(ContextRenderLock&,
                             const lab::AudioBus* source, lab::AudioBus* destination,
                             size_t framesToProcess) override
        {
            if (!numberOfChannels())
                return;
            
            const float* carrierP = source->channelByType(Channel::Left)->data();
            const float* modP = source->channelByType(Channel::Right)->data();

            if (!modP && carrierP) 
            {
                destination->copyFrom(*source);
            }
            else 
            {
                float* destP = destination->channel(0)->mutableData();
                size_t n = framesToProcess;
                while (n--)
                {
                    float carrier = *carrierP++;
                    float mod = *modP++;
                    *destP++ = (carrier > mod) ? 1.0f : -1.0f;
                }
            }
        }

        virtual void reset() override { }

        virtual double tailTime() const override { return 0; }
        virtual double latencyTime() const override { return 0; }
    };

    ////////////////////
    // Public PWMNode //
    ////////////////////

    PWMNode::PWMNode(float sampleRate) : lab::AudioBasicProcessorNode(sampleRate)
    {
        m_processor.reset(new PWMNodeInternal(sampleRate));

        internalNode = static_cast<PWMNodeInternal*>(m_processor.get()); // Currently unused 

        setNodeType(lab::NodeType::NodeTypePWM);

        addInput(std::unique_ptr<AudioNodeInput>(new lab::AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new lab::AudioNodeOutput(this, 2))); 

        initialize();
    }

    PWMNode::~PWMNode()
    {
        uninitialize();
    }

    
}

