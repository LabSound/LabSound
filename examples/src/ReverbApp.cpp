
#include "LabSound.h"

// webaudio specific headers
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

// Examples
#include "ReverbSample.h"

using namespace LabSound;

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();

    reverbSample(context, 10.0f);
    
    return 0;
}
