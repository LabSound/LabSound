// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"

#include "LabSound/extended/ClipNode.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioBus.h"
#include "internal/VectorMath.h"

#include <WTF/MathExtras.h>

#include <iostream>
#include <vector>

using namespace lab;

namespace lab
{

    /////////////////////////////////////
    // Prviate ClipNode Implementation //
    /////////////////////////////////////

    class ClipNode::ClipNodeInternal : public lab::AudioProcessor {
    public:

        ClipNodeInternal(float sampleRate) : AudioProcessor(sampleRate, 2), mode(ClipNode::CLIP)
        {
            auto fMax = std::numeric_limits<float>::max();
            aVal = std::make_shared<AudioParam>("a", -1.0, -fMax, fMax);
            bVal = std::make_shared<AudioParam>("b",  1.0, -fMax, fMax);
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

            unsigned numChannels = numberOfChannels();

            if (mode == ClipNode::TANH)
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

        virtual double tailTime() const override { return 0; }
        virtual double latencyTime() const override { return 0; }

        ClipNode::Mode mode;

        std::shared_ptr<AudioParam> aVal;
        std::shared_ptr<AudioParam> bVal;

        std::vector<float> gainValues;
    };

    /////////////////////
    // Public ClipNode //
    /////////////////////

    ClipNode::ClipNode(float sampleRate) : lab::AudioBasicProcessorNode(sampleRate)
    {
        m_processor.reset(new ClipNodeInternal(sampleRate));

        internalNode = static_cast<ClipNodeInternal*>(m_processor.get());

        setNodeType(lab::NodeType::NodeTypeClip);

        m_params.push_back(internalNode->aVal);
        m_params.push_back(internalNode->bVal);

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
        internalNode->mode = m;
    }

    std::shared_ptr<AudioParam> ClipNode::aVal() { return internalNode->aVal; }
    std::shared_ptr<AudioParam> ClipNode::bVal() { return internalNode->bVal; }

} // end namespace lab

