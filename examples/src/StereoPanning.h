#include "ExampleBaseApp.h"

// Illustrates the use of simple Equal-power stereo panning
struct StereoPanningApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = LabSound::init();
        auto ac = context.get();
        
        SoundBuffer train("samples/trainrolling.wav", context->sampleRate());
        auto stereoPanner = std::make_shared<StereoPannerNode>(context->sampleRate());
        
        std::shared_ptr<AudioBufferSourceNode> trainNode;
        {
            ContextGraphLock g(context, "spatialization");
            ContextRenderLock r(context, "spatialization");
            stereoPanner->connect(ac, context->destination().get(), 0, 0);
            trainNode = train.play(r, stereoPanner, 0.0f);
        }
        
        if (trainNode)
        {
            trainNode->setLooping(true);
            
            const int seconds = 5;
            float halfTime = seconds * 0.5f;
            for (float i = 0; i < seconds; i += 0.01f)
            {
                float x = (i - halfTime) / halfTime;
                // Put position a +up && +front, because if it goes right through the
                // listener at (0, 0, 0) it abruptly switches from left to right.
                stereoPanner->pan()->setValue(x);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        else
        {
            std::cerr << std::endl << "Couldn't initialize train node to play" << std::endl;
        }
        
        LabSound::finish(context);
    }
};
