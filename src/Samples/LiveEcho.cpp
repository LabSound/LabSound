

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

void liveEcho(RefPtr<AudioContext> context, float seconds)
{
    ExceptionCode ec;
    //--------------------------------------------------------------
    // Send live audio straight to the output
    //
    RefPtr<MediaStreamAudioSourceNode> input = context->createMediaStreamSource(new MediaStream(), ec);
    input->connect(context->destination(), 0, 0, ec);
    std::cout << "Starting echo" << std::endl;
    
    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
    std::cout << "Ending echo" << std::endl;
}
