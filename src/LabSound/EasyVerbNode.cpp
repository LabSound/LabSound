// Copyright (c) 2014 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound.h"
#include "EasyVerbNode.h"

#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessor.h"
#include <audio/VectorMath.h>

#include <iostream>
#include <vector>

using namespace WebCore;
using namespace WebCore::VectorMath;
using namespace std;

namespace LabSound {

	class EasyVerbNode::NodeInternal : public WebCore::AudioProcessor{
	public:

		NodeInternal(WebCore::AudioContext* context, float sampleRate) : AudioProcessor(sampleRate), channels(2) {

			m_delayTime = AudioParam::create(context, "delayTime", 1, 0.1, 24);

		}

		virtual ~NodeInternal() {
		}

		// AudioProcessor interface
		virtual void initialize() { }
		virtual void uninitialize() { }

		// Processes the source to destination bus.  The number of channels must match in source and destination.
		void process(const WebCore::AudioBus* sourceBus, WebCore::AudioBus* destinationBus, size_t framesToProcess) {

			perryVerb.setT60(m_delayTime->value());

			const float *source = sourceBus->channel(0)->data();

			float* dLeft = destinationBus->channel(0)->mutableData();
			float* dRight = destinationBus->channel(1)->mutableData();

			for (int i = 0; i < framesToProcess; ++i) {
				float dL = perryVerb.tick((double)source[i], 0);
				float dR = perryVerb.tick((double)source[i], 1);
				*dLeft++ = dL;
				*dRight++ = dR;
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

		RefPtr<AudioParam> m_delayTime;

		stk::NRev perryVerb;

	};

	EasyVerbNode::EasyVerbNode(WebCore::AudioContext* context, float sampleRate)
		: WebCore::AudioBasicProcessorNode(context, sampleRate),
		  data(new NodeInternal(context, sampleRate)) {
		
		m_processor = std::move(std::unique_ptr<WebCore::AudioProcessor>(data));

		setNodeType((AudioNode::NodeType) LabSound::NodeTypeUnknown);

		addInput(adoptPtr(new WebCore::AudioNodeInput(this)));
		addOutput(adoptPtr(new WebCore::AudioNodeOutput(this, 1))); // 2 stereo


		initialize(); 

	}

	AudioParam* EasyVerbNode::delayTime() const {
		return data->m_delayTime.get();
	}

	EasyVerbNode::~EasyVerbNode() {

		uninitialize();

	}

} // LabSound

