//
//  STKNode.h
//
// Copyright (c) 2014 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef __LabSound__STKNode__
#define __LabSound__STKNode__

#include "../Modules/webaudio/AudioContext.h"
#include "../Modules/webaudio/AudioNode.h"
#include "../Modules/webaudio/AudioParam.h"

#include "ADSRNode.h"
#include "STKIncludes.h"

#include <direct.h>

namespace LabSound {

	using namespace WebCore;

	template <class InternalSTKType>
	class STKNode : public AudioScheduledSourceNode {

	public:

		static PassRefPtr<STKNode> create(AudioContext* context, float sampleRate) {

			// Oops, Windows platform for now...
			char cwd[MAX_PATH];
			_getcwd(cwd, MAX_PATH);

			std::string resourcePath = std::string(cwd) + std::string("\\stkresources\\");
			stk::Stk::setRawwavePath(resourcePath);
			std::cout << "STK Resource Path: " << resourcePath << std::endl;

			return adoptRef(new STKNode(context, sampleRate));

		}

		STKNode(AudioContext* context, float sampleRate) : AudioScheduledSourceNode(context, sampleRate)  {

			stk::Stk::setSampleRate(sampleRate);

			// addInput(adoptPtr(new AudioNodeInput(this)));

			// Two channels 
			addOutput(adoptPtr(new AudioNodeOutput(this, 1)));

			setNodeType((AudioNode::NodeType) LabSound::NodeTypeSTK);

			initialize();

			std::cout << "Initializing STKNode \n";

			//gainNode = ADSRNode::create(context, sampleRate);
			//LabSound::connect(this->gainNode.get(), this);

		}

		void process(size_t framesToProcess) {

			// First output bus 
			AudioBus* outputBus = output(0)->bus();

			if (!isInitialized() || !outputBus->numberOfChannels()) {
				outputBus->zero();
				return;
			}

			size_t quantumFrameOffset;
			size_t nonSilentFramesToProcess;

			updateSchedulingInfo(framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);

			if (!nonSilentFramesToProcess) {
				outputBus->zero();
				return;
			}

			float* leftChannel = outputBus->channel(0)->mutableData();
			//float* rightChannel = outputBus->channel(1)->mutableData();

			stk::StkFrames synthFrames(framesToProcess, 1);
			synth.tick(synthFrames);

			// ??? 
			for (uint32_t i = 0; i < framesToProcess; i++) {
				leftChannel[i] = float(synthFrames(i, 0));
				// rightChannel[i] = synthFrames(i, 1);
			}

			//AudioBus* inputBus = input(0)->bus();
			//outputBus->copyFrom(*inputBus);
			outputBus->clearSilentFlag();

		}

		InternalSTKType getSynth() { return synth; }

	private:

		//STKNode(WebCore::AudioContext*, float sampleRate);

		// Satisfy the AudioNode interface
		//virtual void process(size_t);
		virtual void reset() { /*m_currentSampleFrame = 0;*/ }

		// virtual double tailTime() const OVERRIDE { return 0; }
		// virtual double latencyTime() const OVERRIDE { return 0; }

		virtual bool propagatesSilence() const OVERRIDE{
			return false;
		}

		RefPtr<ADSRNode> gainNode;

		InternalSTKType synth;

	};
}

#endif
