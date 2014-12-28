#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;

#define USE_LIVE

// Send live audio to a Dalek filter, constructed according to
// the recipe at http://webaudio.prototyping.bbc.co.uk/ring-modulator/
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();

    auto vIn = OscillatorNode::create(context, context->sampleRate());
    vIn->frequency()->setValue(30.0f);
    vIn->start(0);
    
    auto vInGain = GainNode::create(context, context->sampleRate());
    vInGain->gain()->setValue(0.5f);
    
    // GainNodes can take negative gain which represents phase inversion
    auto vInInverter1 = GainNode::create(context, context->sampleRate());
    vInInverter1->gain()->setValue(-1.0f);
    auto vInInverter2 = GainNode::create(context, context->sampleRate());
    vInInverter2->gain()->setValue(-1.0f);
    
    DiodeNode* vInDiode1 = new DiodeNode(context);
    DiodeNode* vInDiode2 = new DiodeNode(context);
    
    auto vInInverter3 = GainNode::create(context, context->sampleRate());
    vInInverter3->gain()->setValue(-1.0f);
    
    // Now we create the objects on the Vc side of the graph
#ifndef USE_LIVE
    SoundBuffer sample(context, "human-voice.mp4");
    auto player = sample.create();
#endif
    
    auto vcInverter1 = GainNode::create(context, context->sampleRate());
    vcInverter1->gain()->setValue(-1.0f);
    
    DiodeNode* vcDiode3 = new DiodeNode(context);
    DiodeNode* vcDiode4 = new DiodeNode(context);
    
    // A gain node to control master output levels
    auto outGain = GainNode::create(context, context->sampleRate());
    outGain->gain()->setValue(4.0f);
    
    // A small addition to the graph given in Parker's paper is a compressor node
    // immediately before the output. This ensures that the user's volume remains
    // somewhat constant when the distortion is increased.
    auto compressor = DynamicsCompressorNode::create(context, context->sampleRate());
    compressor->threshold()->setValue(-12.0f);
    
    // Now we connect up the graph following the block diagram above (on the web page).
    // When working on complex graphs it helps to have a pen and paper handy!
    
    // First the Vc side
#ifdef USE_LIVE
    auto input = context->createMediaStreamSource(context, ec);
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
    outGain->connect(context->destination().get(), 0, 0, ec);
    
#ifndef USE_LIVE
    player->start(0);
#endif
    
    const int seconds = 10;
    for (int t = 0; t < seconds; ++t)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    LabSound::finish(context);
    
    return 0;
}