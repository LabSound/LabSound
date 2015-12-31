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
        auto context = lab::MakeAudioContext();
        float sampleRate = context->sampleRate();
        
#ifndef USE_LIVE
        SoundBuffer sample("samples/voice.ogg", sampleRate);
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
        std::shared_ptr<AudioBufferSourceNode> player;
        
        {
            ContextGraphLock g(context, "dalek voice");
            ContextRenderLock r(context, "dalek voice");
            
            vIn = std::make_shared<OscillatorNode>(r, sampleRate);
            vIn->frequency()->setValue(30.0f);
            vIn->start(0);
            
            vInGain = std::make_shared<GainNode>(sampleRate);
            vInGain->gain()->setValue(0.5f);
            
            // GainNodes can take negative gain which represents phase inversion
            vInInverter1 = std::make_shared<GainNode>(sampleRate);
            vInInverter1->gain()->setValue(-1.0f);
            vInInverter2 = std::make_shared<GainNode>(sampleRate);
            vInInverter2->gain()->setValue(-1.0f);
            
            vInDiode1 = std::make_shared<DiodeNode>(r, sampleRate);
            vInDiode2 = std::make_shared<DiodeNode>(r, sampleRate);
            
            vInInverter3 = std::make_shared<GainNode>(sampleRate);
            vInInverter3->gain()->setValue(-1.0f);
            
            // Now we create the objects on the Vc side of the graph
#ifndef USE_LIVE
            player = sample.create(r, sampleRate);
            if (!player)
            {
                std::cerr << "Sample buffer wasn't loaded" << std::endl;
                return;
            }
#endif
            
            vcInverter1 = std::make_shared<GainNode>(sampleRate);
            vcInverter1->gain()->setValue(-1.0f);
            
            vcDiode3 = std::make_shared<DiodeNode>(r, sampleRate);
            vcDiode4 = std::make_shared<DiodeNode>(r, sampleRate);
            
            // A gain node to control master output levels
            outGain = std::make_shared<GainNode>(sampleRate);
            outGain->gain()->setValue(1.0f);
            
            // A small addition to the graph given in Parker's paper is a compressor node
            // immediately before the output. This ensures that the user's volume remains
            // somewhat constant when the distortion is increased.
            compressor = std::make_shared<DynamicsCompressorNode>(sampleRate);
            compressor->threshold()->setValue(-12.0f);
            
            // Now we connect up the graph following the block diagram above (on the web page).
            // When working on complex graphs it helps to have a pen and paper handy!
            
            // First the Vc side
            AudioContext* ac = context.get();
            
#ifdef USE_LIVE
            input = MakeHardwareSourceNode(r);
            input->connect(ac, vcInverter1.get(), 0, 0);
            input->connect(ac, vcDiode4->node().get(), 0, 0);
#else
            player->connect(ac, vcInverter1.get(), 0, 0);
            player->connect(ac, vcDiode4->node().get(), 0, 0);
#endif
            
            vcInverter1->connect(ac, vcDiode3->node().get(), 0, 0);
            
            // Then the Vin side
            vIn->connect(ac, vInGain.get(), 0, 0);
            vInGain->connect(ac, vInInverter1.get(), 0, 0);
            vInGain->connect(ac, vcInverter1.get(), 0, 0);
            vInGain->connect(ac, vcDiode4->node().get(), 0, 0);
            
            vInInverter1->connect(ac, vInInverter2.get(), 0, 0);
            vInInverter1->connect(ac, vInDiode2->node().get(), 0, 0);
            vInInverter2->connect(ac, vInDiode1->node().get(), 0, 0);
            
            // Finally connect the four diodes to the destination via the output-stage compressor and master gain node
            vInDiode1->node()->connect(ac, vInInverter3.get(), 0, 0);
            vInDiode2->node()->connect(ac, vInInverter3.get(), 0, 0);
            
            vInInverter3->connect(ac, compressor.get(), 0, 0);
            vcDiode3->node()->connect(ac, compressor.get(), 0, 0);
            vcDiode4->node()->connect(ac, compressor.get(), 0, 0);
            
            compressor->connect(ac, outGain.get(), 0, 0);
            outGain->connect(ac, context->destination().get(), 0, 0);
            
#ifndef USE_LIVE
            player->start(0);
#endif
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(30));
        lab::CleanupAudioContext(context);
    }
};