#include "ExampleBaseApp.h"

struct MicrophoneLoopbackApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = LabSound::init();
        std::shared_ptr<AudioHardwareSourceNode> input;
        
        {
            ContextGraphLock g(context, "Mic Loopback");
            ContextRenderLock r(context, "Mic Loopback");
            
            input = MakeHardwareSourceNode(r);
            input->connect(context.get(), context->destination().get(), 0, 0);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
        LabSound::finish(context);
    }
};
