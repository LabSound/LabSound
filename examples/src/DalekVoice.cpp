//
//  DalekVoice.cpp
//  LabSound
//
//  Created by Nick Porcino on 2012 12/19.
//
//

#include "LabSoundConfig.h"
#include "DalekVoice.h"

#include "DiodeNode.h"
#include "DynamicsCompressorNode.h"
#include "GainNode.h"
#include "MediaStreamAudioSourceNode.h"
#include "OscillatorNode.h"
#include "SoundBuffer.h"

#include <iostream>

using namespace WebCore;
using namespace LabSound;

#define USE_LIVE

void dalekVoice(RefPtr<AudioContext> context, float seconds)
{
    ExceptionCode ec;
    //--------------------------------------------------------------
    // Send live audio to a Dalek filter, constructed according to
    // the recipe at http://webaudio.prototyping.bbc.co.uk/ring-modulator/
    //

    RefPtr<OscillatorNode> vIn = context->createOscillator();
    vIn->frequency()->setValue(30.0f);
    vIn->start(0);

    RefPtr<GainNode> vInGain = context->createGain();
    vInGain->gain()->setValue(0.5f);

    // GainNodes can take negative gain which represents phase inversion
    RefPtr<GainNode> vInInverter1 = context->createGain();
    vInInverter1->gain()->setValue(-1.0f);
    RefPtr<GainNode> vInInverter2 = context->createGain();
    vInInverter2->gain()->setValue(-1.0f);
        
    DiodeNode* vInDiode1 = new DiodeNode(context.get());
    DiodeNode* vInDiode2 = new DiodeNode(context.get());
    
    RefPtr<GainNode> vInInverter3 = context->createGain();
    vInInverter3->gain()->setValue(-1.0f);

    // Now we create the objects on the Vc side of the graph
#ifndef USE_LIVE
    SoundBuffer sample(context, "human-voice.mp4");
    RefPtr<WebCore::AudioBufferSourceNode> player = sample.create();
#endif
    
    RefPtr<GainNode> vcInverter1 = context->createGain();
    vcInverter1->gain()->setValue(-1.0f);
    
    DiodeNode* vcDiode3 = new DiodeNode(context.get());
    DiodeNode* vcDiode4 = new DiodeNode(context.get());
    
    // A gain node to control master output levels
    RefPtr<GainNode> outGain = context->createGain();
    outGain->gain()->setValue(4.0f);

    // A small addition to the graph given in Parker's paper is a compressor node
    // immediately before the output. This ensures that the user's volume remains
    // somewhat constant when the distortion is increased.

    RefPtr<DynamicsCompressorNode> compressor = context->createDynamicsCompressor();
    compressor->threshold()->setValue(-12.0f);

    // Now we connect up the graph following the block diagram above (on the web page).
    // When working on complex graphs it helps to have a pen and paper handy!
    
    // First the Vc side
#ifdef USE_LIVE
    RefPtr<MediaStreamAudioSourceNode> input = context->createMediaStreamSource(ec);
    input->connect(vcInverter1.get(), 0, 0, ec);
    input->connect(vcDiode4->node().get(), 0, 0, ec);
#else
    player->connect(vcInverter1.get(), 0, 0, ec);
    player->connect(vcDiode4->node().get(), 0, 0, ec);
#endif
    
    vcInverter1->connect(vcDiode3->node().get(), 0, 0, ec);

    // Then the Vin side
    
    vIn->connect(vInGain.get(), 0, 0, ec);
    vInGain->connect(vInInverter1.get(), 0, 0, ec);
    vInGain->connect(vcInverter1.get(), 0, 0, ec);
    vInGain->connect(vcDiode4->node().get(), 0, 0, ec);
    
    vInInverter1->connect(vInInverter2.get(), 0, 0, ec);
    vInInverter1->connect(vInDiode2->node().get(), 0, 0, ec);
    vInInverter2->connect(vInDiode1->node().get(), 0, 0, ec);

    // Finally connect the four diodes to the destination via the output-stage compressor and master gain node
    
    vInDiode1->node().get()->connect(vInInverter3.get(), 0, 0, ec);
    vInDiode2->node().get()->connect(vInInverter3.get(), 0, 0, ec);
    
    vInInverter3->connect(compressor.get(), 0, 0, ec);
    vcDiode3->node().get()->connect(compressor.get(), 0, 0, ec);
    vcDiode4->node().get()->connect(compressor.get(), 0, 0, ec);
    
    compressor->connect(outGain.get(), 0, 0, ec);
    outGain->connect(context->destination(), 0, 0, ec);
    
#ifndef USE_LIVE
    player->start(0);
#endif
    
    std::cout << "Starting echo" << std::endl;
    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
    std::cout << "Ending echo" << std::endl;
}

