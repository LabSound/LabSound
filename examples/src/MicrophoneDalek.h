// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

//#define USE_LIVE

#include "ExampleBaseApp.h"

struct MicrophoneDalekApp : public LabSoundExampleApp
{
    
    // Send live audio to a Dalek filter, constructed according to
    // the recipe at http://webaudio.prototyping.bbc.co.uk/ring-modulator/
    void PlayExample()
    {
        auto context = lab::MakeRealtimeAudioContext();
        
#ifndef USE_LIVE
        std::shared_ptr<AudioBus> audioClip = MakeBusFromFile("samples/voice.ogg", false);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>();
#endif
        
        std::shared_ptr<AudioHardwareSourceNode> input;
        
        std::shared_ptr<OscillatorNode> vIn;
        std::shared_ptr<GainNode> vInGain;
        std::shared_ptr<GainNode> vInInverter1;
        std::shared_ptr<GainNode> vInInverter2;
        std::shared_ptr<GainNode> vInInverter3;
        std::shared_ptr<DiodeNode> vInDiode1;
        std::shared_ptr<DiodeNode> vInDiode2;
        std::shared_ptr<GainNode> vcInverter1;
        std::shared_ptr<DiodeNode> vcDiode3;
        std::shared_ptr<DiodeNode> vcDiode4; 
        std::shared_ptr<GainNode> outGain;
        std::shared_ptr<DynamicsCompressorNode> compressor;
        
        {
            ContextRenderLock r(context.get(), "dalek voice");
            
            vIn = std::make_shared<OscillatorNode>(context->sampleRate());
            vIn->frequency()->setValue(30.0f);
            vIn->start(0.f);
            
            vInGain = std::make_shared<GainNode>();
            vInGain->gain()->setValue(0.5f);
            
            // GainNodes can take negative gain which represents phase inversion
            vInInverter1 = std::make_shared<GainNode>();
            vInInverter1->gain()->setValue(-1.0f);
            vInInverter2 = std::make_shared<GainNode>();
            vInInverter2->gain()->setValue(-1.0f);
            
            vInDiode1 = std::make_shared<DiodeNode>();
            vInDiode2 = std::make_shared<DiodeNode>();
            
            vInInverter3 = std::make_shared<GainNode>();
            vInInverter3->gain()->setValue(-1.0f);
            
            // Now we create the objects on the Vc side of the graph
            vcInverter1 = std::make_shared<GainNode>();
            vcInverter1->gain()->setValue(-1.0f);
            
            vcDiode3 = std::make_shared<DiodeNode>();
            vcDiode4 = std::make_shared<DiodeNode>();
            
            // A gain node to control master output levels
            outGain = std::make_shared<GainNode>();
            outGain->gain()->setValue(1.0f);
            
            // A small addition to the graph given in Parker's paper is a compressor node
            // immediately before the output. This ensures that the user's volume remains
            // somewhat constant when the distortion is increased.
            compressor = std::make_shared<DynamicsCompressorNode>();
            compressor->threshold()->setValue(-14.0f);
            
            // Now we connect up the graph following the block diagram above (on the web page).
            // When working on complex graphs it helps to have a pen and paper handy!
            
#ifdef USE_LIVE
            input = MakeHardwareSourceNode(r);
            context->connect(vcInverter1, input, 0, 0);
            context->connect(vcDiode4->node(), input, 0, 0);
#else
            audioClipNode->setBus(r, audioClip);
            //context->connect(vcInverter1, audioClipNode, 0, 0); // dimitri 
            context->connect(vcDiode4->node(), audioClipNode, 0, 0);
            audioClipNode->start(0.f);
#endif
            
            context->connect(vcDiode3->node(), vcInverter1, 0, 0);
            
            // Then the Vin side
            context->connect(vInGain, vIn, 0, 0);
            context->connect(vInInverter1, vInGain, 0, 0);
            context->connect(vcInverter1, vInGain, 0, 0);
            context->connect(vcDiode4->node(), vInGain, 0, 0);
            
            context->connect(vInInverter2, vInInverter1, 0, 0);
            context->connect(vInDiode2->node(), vInInverter1, 0, 0);
            context->connect(vInDiode1->node(), vInInverter2, 0, 0);
            
            // Finally connect the four diodes to the destination via the output-stage compressor and master gain node
            context->connect(vInInverter3, vInDiode1->node(), 0, 0);
            context->connect(vInInverter3, vInDiode2->node(), 0, 0);
            
            context->connect(compressor, vInInverter3, 0, 0);
            context->connect(compressor, vcDiode3->node(), 0, 0);
            context->connect(compressor, vcDiode4->node(), 0, 0);
            
            context->connect(outGain, compressor, 0, 0);
            context->connect(context->destination(), outGain, 0, 0);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
};