// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifdef _MSC_VER
#include <windows.h>
std::string PrintCurrentDirectory()
{
    char buffer[MAX_PATH] = { 0 };
    GetCurrentDirectory(MAX_PATH, buffer);
    return std::string(buffer);
}
#endif

#include "ExampleBaseApp.h"
#include "LabSound/extended/BPMDelay.h"
#include "LabSound/extended/PingPongDelayNode.h"

// An example with a bunch of nodes to verify api + functionality changes/improvements/regressions
struct ValidationApp : public LabSoundExampleApp
{

    void PlayExample()
    {
        
        std::array<int, 8> majorScale = {0, 2, 4, 5, 7, 9, 11, 12};
        std::array<int, 8> naturalMinorScale = {0, 2, 3, 5, 7, 9, 11, 12};
        std::array<int, 6> pentatonicMajor = { 0, 2, 4, 7, 9, 12 };
        std::array<int, 8> pentatonicMinor = { 0, 3, 5, 7, 10, 12 };
        std::array<int, 8> delayTimes = { 266, 533, 399 };
    
        auto randomFloat = std::uniform_real_distribution<float>(0, 1);
        auto randomScaleDegree = std::uniform_int_distribution<int>(0, pentatonicMajor.size() - 1);
        auto randomTimeIndex = std::uniform_int_distribution<int>(0, delayTimes.size() - 1);
        
        //std::cout << "Current Directory: " << PrintCurrentDirectory() << std::endl;
        
        auto context = lab::MakeAudioContext();
        auto ac = context.get();

        std::shared_ptr<AudioBufferSourceNode> beatNode;
        std::shared_ptr<PingPongDelayNode> pingping;

        {
            ContextGraphLock g(context, "Validator");
            ContextRenderLock r(context, "Validator");
            
            pingping  = std::make_shared<PingPongDelayNode>(context->sampleRate(), 120.0f);
            pingping->BuildSubgraph(g);
            pingping->SetFeedback(0.5);
            pingping->SetDelayIndex(lab::TempoSync::TS_16T);

            pingping->output->connect(ac, context->destination().get(), 0, 0);

            SoundBuffer beat("samples/kick.wav", context->sampleRate());
            beatNode = beat.play(r, pingping->input, 0.0f);
        }
     
        const int seconds = 10;
        for (int i = 0; i < seconds; i++)
        {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
       
        lab::CleanupAudioContext(context);
    }
};
