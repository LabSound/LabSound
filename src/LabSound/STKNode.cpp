#include "STKNode.h"

#include "../Modules/webaudio/OscillatorNode.h"

#include "LabSound.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "STKIncludes.h"

using namespace WebCore;

namespace LabSound {

    STKNode::STKNode(AudioContext* context, float sampleRate) : AudioNode(context, sampleRate)  {

        addInput(adoptPtr(new AudioNodeInput(this)));
        addOutput(adoptPtr(new AudioNodeOutput(this, 1)));

		setNodeType((AudioNode::NodeType) LabSound::NodeTypeSTK);

		gainNode = ADSRNode::create(context, sampleRate); 

        LabSound::connect(this->gainNode.get(), this);

    }

    void STKNode::process(size_t framesToProcess) {

        AudioBus* outputBus = output(0)->bus();

        if (!isInitialized() || !outputBus->numberOfChannels()) {
            outputBus->zero();
            return;
        }

        AudioBus* inputBus = input(0)->bus();
        outputBus->copyFrom(*inputBus);
        outputBus->clearSilentFlag();

    }

    void STKNode::update() {

    }

    AudioParam* STKNode::attack()    const { 
		return gainNode->attackTime(); 
	}

    AudioParam* STKNode::decay()     const {
		return gainNode->decayTime(); 
	}

    AudioParam* STKNode::sustain()   const { 
		return gainNode->sustainLevel(); 
	}

    AudioParam* STKNode::release()   const { 
		return gainNode->releaseTime();
	}

    void STKNode::noteOn()  {
        gainNode->noteOn();
    }

    void STKNode::noteOff() {
        gainNode->noteOff(); 
    }

    bool STKNode::propagatesSilence() const {
        return gainNode->propagatesSilence();
    }

} // namespace LabSound
