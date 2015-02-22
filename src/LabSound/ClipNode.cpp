
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound.h"
#include "ClipNode.h"

#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessor.h"
#include "VectorMath.h"
#include "AudioContextLock.h"

#include <iostream>
#include <vector>

using namespace WebCore;
using namespace WebCore::VectorMath;
using namespace std;

namespace LabSound {

    class ClipNode::ClipNodeInternal : public WebCore::AudioProcessor {
    public:

        ClipNodeInternal(float sampleRate)
        : AudioProcessor(sampleRate)
        , numChannels(1)
        , mode(ClipNode::CLIP)
        {
            aVal = make_shared<AudioParam>("a", -1.0, -FLT_MAX, FLT_MAX);
            bVal = make_shared<AudioParam>("b",  1.0, -FLT_MAX, FLT_MAX);
        }

        virtual ~ClipNodeInternal() {}

        // AudioProcessor interface
        virtual void initialize() {
        }

        virtual void uninitialize() { }

        // Processes the source to destination bus.  The number of channels must match in source and destination.
        virtual void process(ContextGraphLock& g, ContextRenderLock& r,
                             const WebCore::AudioBus* sourceBus, WebCore::AudioBus* destinationBus, size_t framesToProcess) {
            if (!numChannels)
                return;

            // We handle both the 1 -> N and N -> N case here.
            const float* source = sourceBus->channel(0)->data();

            // this will only ever happen once, so if heap contention is an issue it should only ever cause one glitch
            // what would be better, alloca? What does webaudio do elsewhere for this sort of thing?
            if (gainValues.size() < framesToProcess)
                gainValues.resize(framesToProcess);

            if (mode == ClipNode::TANH) {
                float outputGain = aVal->value(r.contextPtr());
                float inputGain = bVal->value(r.contextPtr());
                for (unsigned int channelIndex = 0; channelIndex < numChannels; ++channelIndex) {
                    if (sourceBus->numberOfChannels() == numChannels)
                        source = sourceBus->channel(channelIndex)->data();
                    float* destination = destinationBus->channel(channelIndex)->mutableData();
                    for (size_t i = 0; i < framesToProcess; ++i) {
                        *destination++ = outputGain * tanhf(inputGain * source[i]);
                    }
                }
            }
            else {
                float minf = aVal->value(r.contextPtr());
                float maxf = bVal->value(r.contextPtr());
                for (unsigned int channelIndex = 0; channelIndex < numChannels; ++channelIndex) {
                    if (sourceBus->numberOfChannels() == numChannels)
                        source = sourceBus->channel(channelIndex)->data();
                    float* destination = destinationBus->channel(channelIndex)->mutableData();
                    for (size_t i = 0; i < framesToProcess; ++i) {
                        float d = source[i];
                        if (d < minf) d = minf;
                        else if (d > maxf) d = maxf;
                        *destination++ = d;
                    }
                }
            }
        }

        // Resets filter state
        virtual void reset() { }

        virtual void setNumberOfChannels(unsigned i) {
            numChannels = i;
        }

        virtual double tailTime() const { return 0; }
        virtual double latencyTime() const { return 0; }

        unsigned int numChannels;
        ClipNode::Mode mode;
		std::shared_ptr<AudioParam> aVal;
		std::shared_ptr<AudioParam> bVal;
        vector<float> gainValues;
    };

    ClipNode::ClipNode(float sampleRate)
    : WebCore::AudioBasicProcessorNode(sampleRate)
    , data(new ClipNodeInternal(sampleRate))
    {
        m_processor = std::move(std::unique_ptr<WebCore::AudioProcessor>(data));

        setNodeType((AudioNode::NodeType) LabSound::NodeTypeClip);

        addInput(std::unique_ptr<AudioNodeInput>(new WebCore::AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new WebCore::AudioNodeOutput(this, 2))); // 2 stereo

        initialize();
    }
    
    ClipNode::~ClipNode() {
        data->numChannels = 0;  // ensure there if there is a latent callback pending, pd is not invoked
        //delete data; // not deleting it because the unique_ptr will take care of that
        data = 0;
        uninitialize();
    }

    void ClipNode::setMode(Mode m) {
        data->mode = m;
    }

    std::shared_ptr<AudioParam> ClipNode::aVal() { return data->aVal; }
    std::shared_ptr<AudioParam> ClipNode::bVal() { return data->bVal; }




} // LabSound

