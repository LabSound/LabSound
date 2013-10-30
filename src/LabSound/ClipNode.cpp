
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "ClipNode.h"

#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessor.h"

#include <iostream>

using namespace WebCore;

namespace LabSound {

    class ClipNode::ClipNodeInternal : public WebCore::AudioProcessor {
    public:

        ClipNodeInternal(WebCore::AudioContext* context, float sampleRate)
        : AudioProcessor(sampleRate)
        , numChannels(2)
        , mode(ClipNode::CLIP)
        {
            minVal = AudioParam::create(context, "min", -1.0, -FLT_MAX, FLT_MAX);
            maxVal = AudioParam::create(context, "max",  1.0, -FLT_MAX, FLT_MAX);
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

            float minf = minVal->value();
            float maxf = maxVal->value();

            if (mode == ClipNode::CLIP)
                while (n--) {
                    float d = *srcP++;
                    if (d < minf) d = minf;
                    else if (d > maxf) d = maxf;
                    *destP++ = d;
                }
            else {
                // decide how to interpret min and max if at all for tanh
                while (n--) {
                    *destP++ = tanhf(*srcP++);
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

        int numChannels;
        ClipNode::Mode mode;
		RefPtr<AudioParam> minVal;
		RefPtr<AudioParam> maxVal;
    };

    ClipNode::ClipNode(WebCore::AudioContext* context, float sampleRate)
    : WebCore::AudioBasicProcessorNode(context, sampleRate)
    , data(new ClipNodeInternal(context, sampleRate))
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

    AudioParam* ClipNode::minVal() { return data->minVal.get(); }
    AudioParam* ClipNode::maxVal() { return data->maxVal.get(); }




} // LabSound

