
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "ClipNode.h"

#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessor.h"

#include <iostream>

namespace LabSound {

    class ClipNode::ClipNodeInternal : public WebCore::AudioProcessor {
    public:

        ClipNodeInternal(float sampleRate)
        : AudioProcessor(sampleRate)
        , numChannels(2)
        , minVal(-1.0f)
        , maxVal(1.0f)
        , mode(ClipNode::CLIP)
        {
        }

        virtual ~ClipNodeInternal() {
        }

        // AudioProcessor interface
        virtual void initialize() {
        }

        virtual void uninitialize() { }

        // Processes the source to destination bus.  The number of channels must match in source and destination.
        virtual void process(const WebCore::AudioBus* source, WebCore::AudioBus* destination, size_t framesToProcess) {
            if (!numChannels)
                return;

            const float* srcP = source->channel(0)->data();
            float* destP = destination->channel(0)->mutableData();
            size_t n = framesToProcess;
            if (mode == ClipNode::CLIP)
                while (n--) {
                    float d = *srcP++;
                    if (d < minVal) d = minVal;
                    else if (d > maxVal) d = maxVal;
                    *destP++ = d;
                }
            else
                while (n--) {
                    *destP++ = tanhf(*srcP++);
                }
        }

        // Resets filter state
        virtual void reset() { }

        virtual void setNumberOfChannels(unsigned i) {
            numChannels = i;
        }

        virtual double tailTime() const { return 0; }
        virtual double latencyTime() const { return 0; }

        int numChannels;
        ClipNode::Mode mode;
        float minVal;
        float maxVal;
    };

    ClipNode::ClipNode(WebCore::AudioContext* context, float sampleRate)
    : WebCore::AudioBasicProcessorNode(context, sampleRate)
    , data(new ClipNodeInternal(sampleRate))
    {
        m_processor = adoptPtr(data);

        setNodeType(NodeTypeConvolver);  // pretend to be a convolver

        addInput(adoptPtr(new WebCore::AudioNodeInput(this)));
        addOutput(adoptPtr(new WebCore::AudioNodeOutput(this, 2))); // 2 stereo

        initialize();
    }
    
    ClipNode::~ClipNode() {
        data->numChannels = 0;  // ensure there if there is a latent callback pending, pd is not invoked
        data = 0;
        uninitialize();
    }

    void ClipNode::setMode(Mode m) {
        data->mode = m;
    }



} // LabSound

