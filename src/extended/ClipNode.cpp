// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"

#include "LabSound/extended/ClipNode.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/VectorMath.h"

#include <iostream>
#include <vector>

using namespace lab;

namespace lab
{

    /////////////////////////////////////
    // Private ClipNode Implementation //
    /////////////////////////////////////

    class ClipNode::ClipNodeInternal : public lab::AudioProcessor 
    {
    public:

        ClipNodeInternal() : AudioProcessor(2)
        {
            auto fMax = std::numeric_limits<float>::max();
            aVal = std::make_shared<AudioParam>("a", -1.0, -fMax, fMax);
            bVal = std::make_shared<AudioParam>("b",  1.0, -fMax, fMax);
            mode = std::make_shared<AudioSetting>("mode");
            mode->setUint32(static_cast<uint32_t>(ClipNode::CLIP));
        }

        virtual ~ClipNodeInternal() { }

        virtual void initialize() override { }

        virtual void uninitialize() override { }

        // Processes the source to destination bus.  The number of channels must match in source and destination.
        virtual void process(ContextRenderLock& r,
                             const lab::AudioBus* sourceBus, lab::AudioBus* destinationBus,
                             size_t framesToProcess) override
        {
            if (!numberOfChannels())
                return;

            // We handle both the 1 -> N and N -> N case here.
            const float* source = sourceBus->channel(0)->data();

            // this will only ever happen once, so if heap contention is an issue it should only ever cause one glitch
            // what would be better, alloca? What does webaudio do elsewhere for this sort of thing?
            if (gainValues.size() < framesToProcess)
                gainValues.resize(framesToProcess);

            size_t numChannels = numberOfChannels();

            ClipNode::Mode clipMode = static_cast<ClipNode::Mode>(mode->valueUint32());

            if (clipMode == ClipNode::TANH)
            {
                float outputGain = aVal->value(r);
                float inputGain = bVal->value(r);

                for (unsigned int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
                {
                    if (sourceBus->numberOfChannels() == numChannels)
                        source = sourceBus->channel(channelIndex)->data();

                    float * destination = destinationBus->channel(channelIndex)->mutableData();
                    for (size_t i = 0; i < framesToProcess; ++i)
                    {
                        *destination++ = outputGain * tanhf(inputGain * source[i]);
                    }
                }
            }
            else
            {
                float minf = aVal->value(r);
                float maxf = bVal->value(r);

                for (unsigned int channelIndex = 0; channelIndex < numChannels; ++channelIndex)
                {
                    if (sourceBus->numberOfChannels() == numChannels)
                        source = sourceBus->channel(channelIndex)->data();

                    float * destination = destinationBus->channel(channelIndex)->mutableData();

                    for (size_t i = 0; i < framesToProcess; ++i)
                    {
                        float d = source[i];

                        if (d < minf)
                            d = minf;
                        else if (d > maxf)
                            d = maxf;

                        *destination++ = d;
                    }
                }
            }
        }

        virtual void reset() override { }

        virtual double tailTime(ContextRenderLock & r) const override { return 0; }
        virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

        std::shared_ptr<AudioParam> aVal;
        std::shared_ptr<AudioParam> bVal;
        std::shared_ptr<AudioSetting> mode;

        std::vector<float> gainValues;
    };

    /////////////////////
    // Public ClipNode //
    /////////////////////

    ClipNode::ClipNode() 
    : lab::AudioBasicProcessorNode()
    {
        internalNode = new ClipNodeInternal();
        m_processor.reset(internalNode);

        m_params.push_back(internalNode->aVal);
        m_params.push_back(internalNode->bVal);
        m_settings.push_back(internalNode->mode);

        addInput(std::unique_ptr<AudioNodeInput>(new lab::AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new lab::AudioNodeOutput(this, 2)));

        initialize();
    }

    ClipNode::~ClipNode()
    {
        uninitialize();
    }

    void ClipNode::setMode(Mode m)
    {
        internalNode->mode->setUint32(uint32_t(m));
    }

    std::shared_ptr<AudioParam> ClipNode::aVal() { return internalNode->aVal; }
    std::shared_ptr<AudioParam> ClipNode::bVal() { return internalNode->bVal; }

} // end namespace lab

