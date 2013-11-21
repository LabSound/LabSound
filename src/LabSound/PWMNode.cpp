
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound.h"
#include "PWMNode.h"

#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessor.h"

#include <iostream>

using namespace WebCore;

namespace LabSound {

    class PWMNode::PWMNodeInternal : public WebCore::AudioProcessor {
    public:

        PWMNodeInternal(WebCore::AudioContext* context, float sampleRate)
        : AudioProcessor(sampleRate)
        , channels(1)
        {
        }

        virtual ~PWMNodeInternal() {
        }

        // AudioProcessor interface
        virtual void initialize() {
        }

        virtual void uninitialize() { }

        // Processes the source to destination bus.  The number of channels must match in source and destination.
        virtual void process(const WebCore::AudioBus* source, WebCore::AudioBus* destination, size_t framesToProcess) {
            if (!channels)
                return;
            
            const float* carrierP = source->channel(0)->data();
            const float* modP = source->channel(1)->data();

            if (!modP && carrierP) {
                destination->copyFrom(*source);
            }
            else {
                float* destP = destination->channel(0)->mutableData();
                size_t n = framesToProcess;
                while (n--) {
                    float carrier = *carrierP++;
                    float mod = *modP++;
                    *destP++ = (carrier > mod) ? 1.0f : -1.0f;
                }
            }
        }

        // Resets filter state
        virtual void reset() { }

        virtual void setNumberOfChannels(unsigned i) {
            channels = i;
        }

        virtual double tailTime() const { return 0; }
        virtual double latencyTime() const { return 0; }

        int channels;
    };

    PWMNode::PWMNode(WebCore::AudioContext* context, float sampleRate)
    : WebCore::AudioBasicProcessorNode(context, sampleRate)
    , data(new PWMNodeInternal(context, sampleRate))
    {
        m_processor = adoptPtr(data);

        setNodeType((AudioNode::NodeType) LabSound::NodeTypePWM);

        addInput(adoptPtr(new WebCore::AudioNodeInput(this)));
        addOutput(adoptPtr(new WebCore::AudioNodeOutput(this, 2))); // 2 stereo

        initialize();
    }

    PWMNode::~PWMNode() {
        delete data;
        data = 0;
        uninitialize();
    }

    
} // LabSound

