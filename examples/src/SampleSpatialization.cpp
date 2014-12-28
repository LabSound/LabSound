
#include "LabSoundIncludes.h"
#include <iostream>

using namespace LabSound;

void sampleSpatialization(std::shared_ptr<LabSound::AudioContext> context, float seconds)
{
    ExceptionCode ec;
    
    //--------------------------------------------------------------
    // Demonstrate 3d spatialization and doppler shift
    //
    SoundBuffer train(context, "trainrolling.wav");
    auto panner = PannerNode::create(context, context->sampleRate());
    panner->connect(context->destination().get(), 0, 0, ec);
    PassRefPtr<AudioBufferSourceNode> trainNode = train.play(panner.get(), 0.0f);
    if (!trainNode) {
        std::cerr << std::endl << "Couldn't initialize train node to play" << std::endl;
    }
    else {
        trainNode->setLooping(true);
        context->listener()->setPosition(0, 0, 0);
        panner->setVelocity(15, 0, 0);
        float halfTime = seconds * 0.5f;
        for (float i = 0; i < seconds; i += 0.01f) {
            float x = (i - halfTime) / halfTime;
            // keep it a bit up and in front, because if it goes right through the listener
            // at (0, 0, 0) it abruptly switches from left to right.
            panner->setPosition(x, 0.1f, 0.1f);
            usleep(10000);
        }
    }
}
