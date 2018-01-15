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
        
        auto context = lab::MakeRealtimeAudioContext();
        auto ac = context.get();

        std::shared_ptr<AudioBus> audioClip = MakeBusFromFile("samples/cello_pluck/cello_pluck_As0.wav", false);
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>();
        std::shared_ptr<PingPongDelayNode> pingping = std::make_shared<PingPongDelayNode>(context->sampleRate(), 120.0f);

        {
            ContextRenderLock r(ac, "Validator");
            
            pingping->BuildSubgraph(context);
            pingping->SetFeedback(0.5f);
            pingping->SetDelayIndex(lab::TempoSync::TS_8);

            ac->connect(context->destination(), pingping->output, 0, 0);

            audioClipNode->setBus(r, audioClip);

            ac->connect(pingping->input, audioClipNode, 0, 0);
            audioClipNode->start(0.0f);
        }
     
        const int seconds = 10;
        for (int i = 0; i < seconds; i++)
        {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
