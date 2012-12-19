

// For starting the WTF library
#include <wtf/ExportMacros.h>
#include "MainThread.h"

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

#include "LiveEcho.h"

#include <unistd.h>
#include <iostream>

using namespace WebCore;
using LabSound::RecorderNode;
using LabSound::SoundBuffer;

void reverbSample(RefPtr<AudioContext> context, float seconds)
{
    ExceptionCode ec;
    //--------------------------------------------------------------
    // Play a sound file through a reverb convolution
    //
    SoundBuffer ir(context, "impulse-responses/tim-warehouse/cardiod-rear-35-10/cardiod-rear-levelled.wav");
    //SoundBuffer ir(context, "impulse-responses/filter-telephone.wav");
    
    SoundBuffer sample(context, "human-voice.mp4");
    
    RefPtr<ConvolverNode> convolve = context->createConvolver();
    convolve->setBuffer(ir.audioBuffer.get());
    RefPtr<GainNode> wetGain = context->createGain();
    wetGain->gain()->setValue(2.f);
    RefPtr<GainNode> dryGain = context->createGain();
    dryGain->gain()->setValue(1.f);
    
    convolve->connect(wetGain.get(), 0, 0, ec);
    wetGain->connect(context->destination(), 0, 0, ec);
    dryGain->connect(context->destination(), 0, 0, ec);
    
    sample.play(convolve.get(), 0);
    
    std::cout << "Starting convolved echo" << std::endl;
    
    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
    std::cout << "Ending echo" << std::endl;
}
