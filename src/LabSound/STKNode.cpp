#include "STKNode.h"

#include "../Modules/webaudio/OscillatorNode.h"

#include "LabSound.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"


using namespace WebCore;

namespace LabSound {

	STKNode::STKNode(AudioContext* context, float sampleRate) : AudioScheduledSourceNode(context, sampleRate)  {

		stk::Stk::setSampleRate(sampleRate);

        // addInput(adoptPtr(new AudioNodeInput(this)));

		// Two channels 
        addOutput(adoptPtr(new AudioNodeOutput(this, 1)));

		setNodeType((AudioNode::NodeType) LabSound::NodeTypeSTK);

		initialize();

		std::cout << "Initializing STKNode \n";

		// gainNode = ADSRNode::create(context, sampleRate); 

        // LabSound::connect(this->gainNode.get(), this);

    }

    void STKNode::process(size_t framesToProcess) {

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
		for (int i = 0; i < framesToProcess; i++) {
			leftChannel[i] = synthFrames(i, 0);
			// rightChannel[i] = synthFrames(i, 1);
		}

        //AudioBus* inputBus = input(0)->bus();
        //outputBus->copyFrom(*inputBus);
        outputBus->clearSilentFlag();

    }

    void STKNode::update() {

		// controlChange(cc, value)

		// op4 Feedback CC
		synth.controlChange(2, 10);

		// op3 Gain CC
		synth.controlChange(4, 1);

		// LFO Speed CC
		synth.controlChange(11, 2.0);

		// LFO Depth CC
		synth.controlChange(1, 4.0);

    }

	// TODO, hook ADSR back up
    AudioParam* STKNode::attack() const { 
		return gainNode->attackTime(); 
	}

    AudioParam* STKNode::decay() const {
		return gainNode->decayTime(); 
	}

    AudioParam* STKNode::sustain() const { 
		return gainNode->sustainLevel(); 
	}

    AudioParam* STKNode::release() const { 
		return gainNode->releaseTime();
	}

    void STKNode::noteOn(float frequency)  {

		synth.noteOn(frequency, 0.50);

        //gainNode->noteOn();

    }

    void STKNode::noteOff() {

		synth.noteOff(.5);

       // gainNode->noteOff(); 

    }

    bool STKNode::propagatesSilence() const {
		return 0; 
        // return gainNode->propagatesSilence();
    }

} // namespace LabSound
