#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;

// Demonstrate 3d spatialization and doppler shift
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    auto ac = context.get();

    SoundBuffer train("trainrolling.wav", context->sampleRate());
    auto panner = std::make_shared<PannerNode>(context->sampleRate());
    std::shared_ptr<AudioBufferSourceNode> trainNode;
    {
        ContextGraphLock g(context, "spatialization");
        ContextRenderLock r(context, "spatialization");
        panner->connect(ac, context->destination().get(), 0, 0, ec);
        trainNode = train.play(g, r, panner, 0.0f);
    }
    
    if (trainNode)
    {
        trainNode->setLooping(true);
        context->listener()->setPosition(0, 0, 0);
        panner->setVelocity(15, 0, 0);
        
        const int seconds = 10;
        float halfTime = seconds * 0.5f;
        for (float i = 0; i < seconds; i += 0.01f)
        {
            float x = (i - halfTime) / halfTime;
            // Put position a +up && +front, because if it goes right through the listener at (0, 0, 0) it abruptly switches from left to right.
            panner->setPosition(x, 0.1f, 0.1f);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    else
    {
        std::cerr << std::endl << "Couldn't initialize train node to play" << std::endl;
    }
    
    LabSound::finish(context);
    return 0;
}
