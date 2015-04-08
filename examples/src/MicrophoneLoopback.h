#include "ExampleBaseApp.h"

struct MicrophoneLoopbackApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = LabSound::init();
        std::shared_ptr<MediaStreamAudioSourceNode> input;
        
        {
            ContextGraphLock g(context, "Mic Loopback");
            ContextRenderLock r(context, "Mic Loopback");
            
            input = context->createMediaStreamSource(g, r);
            input->connect(context.get(), context->destination().get(), 0, 0, ec);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
        LabSound::finish(context);
    }
};
