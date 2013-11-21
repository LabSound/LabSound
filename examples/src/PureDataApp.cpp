

// webaudio specific headers
#include "LabSound.h"
#include "AudioBufferSourceNode.h"
#include "AudioContext.h"
#include "BiquadFilterNode.h"
#include "ConvolverNode.h"
#include "ExceptionCode.h"
#include "GainNode.h"
#include "MediaStream.h"
#include "MediaStreamAudioSourceNode.h"
#include "OscillatorNode.h"
#include "PannerNode.h"

// LabSound
#include "RecorderNode.h"
#include "SoundBuffer.h"
#include "PdNode.h"

#include "PdBase.hpp"

#include <unistd.h>
#include <iostream>

using LabSound::RecorderNode;
using LabSound::SoundBuffer;

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();
    
    RefPtr<LabSound::PdNode> pdNode = LabSound::PdNode::create(context.get(), 44100);
    RefPtr<GainNode> dryGain = context->createGain();
    dryGain->gain()->setValue(1.f);
    pdNode->connect(dryGain.get(), 0, 0, ec);
    dryGain->connect(context->destination(), 0, 0, ec);

    pd::PdBase pd = pdNode->pd();
    pd::Patch patch = pd.openPatch("test.pd", ".");
    std::cout << patch << std::endl;
    
    std::cout << "Starting pd test" << std::endl;

	// play a tone by sending a list
	// [list tone pitch 72 (
	pd.startMessage();
    pd.addSymbol("pitch");
    pd.addFloat(72);
	pd.finishList("tone");
    pd.sendBang("tone");

    float seconds = 10.0f;
    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
    std::cout << "Ending pd test" << std::endl;

    return 0;
}
