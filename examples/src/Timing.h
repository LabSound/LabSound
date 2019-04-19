// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"
#include <iostream>

struct TimingApp : public LabSoundExampleApp
{
    // This sample simply counts time, and checks that chrono's high_resolution_clock
    // and the time measure from the audio destination node essentially agree
    //
    void PlayExample()
    {
        std::unique_ptr<lab::AudioContext> context = lab::MakeRealtimeAudioContext(lab::Channels::Stereo);

        // On Windows, it takes almost 400ms before the audio callbacks
        // begin, so wait for the start up to elapse.
        while (!context->destination()->currentSampleFrame())
        {
            Wait(std::chrono::milliseconds(1));
        }

        auto start_tp = std::chrono::high_resolution_clock::now();
        double start_ac = context->destination()->currentSampleTime();

        std::vector<int64_t> chrono_measure(1000);
        std::vector<int64_t> audio_measure(1000);
        std::vector<int64_t> audio_frame_count(1000);
        for (int i = 0; i < 1000; ++i)
        {
            auto tp2 = std::chrono::high_resolution_clock::now();
            auto ac2 = context->destination()->currentSampleTime();
            auto int_ms = std::chrono::duration_cast<std::chrono::microseconds>(tp2 - start_tp);
            chrono_measure[i] = static_cast<int64_t>(int_ms.count());
            audio_measure[i] = static_cast<int64_t>(ac2 * 1000000);
            audio_frame_count[i] = context->destination()->currentSampleFrame();
            Wait(std::chrono::milliseconds(1));
        }
    }
};
