// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

// Unexpected Token from Wavepot. Shows the utility of LabSound as an experimental playground for DSP.
// Original by Stagas: http://wavepot.com/stagas/unexpected-token (MIT License)

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#include <math.h>
#endif

#include "ExampleBaseApp.h"
#include <cmath>
#include <algorithm>

float note(int n, int octave = 0)
{
    return std::pow(2.0f, (n - 33.f + (12.f * octave)) / 12.0f) * 440.f;
}

std::vector<std::vector<int>> bassline =
{
    {7, 7, 7, 12, 10, 10, 10, 15},
    {7, 7, 7, 15, 15, 17, 10, 29},
    {7, 7, 7, 24, 10, 10, 10, 19},
    {7, 7, 7, 15, 29, 24, 15, 10}
};

std::vector<int> melody =
{
    7, 15, 7, 15,
    7, 15, 10, 15,
    10, 12, 24, 19,
    7, 12, 10, 19
};

std::vector<std::vector<int>> chords = { {7, 12, 17, 10}, {10, 15, 19, 24} };

float quickSin(float x, float t)
{
    return std::sin(2.0f * M_PI * t * x);
}

float quickSaw(float x, float t)
{
    return 1.0f - 2.0f * fmod(t, (1.f / x)) * x;
}

float quickSqr(float x, float t)
{
    return quickSin(x, t) > 0 ? 1.f : -1.f;
}

// perc family of functions implement a simple attack/decay, creating
// a short & percussive envelope for the signal
float perc(float wave, float decay, float o, float t)
{
    float env = std::max(0.f, 0.889f - (o * decay) / ((o * decay) + 1.f));
    auto ret = wave * env;
    return ret;
}

float perc_b(float wave, float decay, float o, float t)
{
    float env = std::min(0.f, 0.950f - (o * decay) / ((o * decay) + 1.f));
    auto ret = wave * env;
    return ret;
}

float hardClip(float n, float x)
{
    return x > n ? n : x < -n ? -n : x;
}

struct FastLowpass
{
    float v = 0;
    float operator() (float n, float input)
    {
        return v += (input - v) / n;
    }
};

struct FastHighpass
{
    float v = 0;
    float operator() (float n, float input)
    {
      return v += input - v * n;
    }
};

// http://www.musicdsp.org/showone.php?id=24
// A Moog-style 24db resonant lowpass
struct MoogFilter
{
    float y1 = 0;
    float y2 = 0;
    float y3 = 0;
    float y4 = 0;
    
    float oldx = 0;
    float oldy1 = 0;
    float oldy2 = 0;
    float oldy3 = 0;
    
    float p, k, t1, t2, r, x;
    
    const float sampleRate = 44100.00f;
    
    float process(double cutoff, double resonance, double sample)
    {
        cutoff = 2.0 * cutoff / sampleRate;
        
        p = cutoff * (1.8 - 0.8 * cutoff);
        k = 2.0 * std::sin(cutoff * M_PI * 0.5) - 1.0;
        t1 = (1.0 - p) * 1.386249;
        t2 = 12.0 + t1 * t1;
        r = resonance * (t2 + 6.0 * t1) / (t2 - 6.0 * t1);
        
        x = sample - r * y4;
        
        // Four cascaded one-pole filters (bilinear transform)
        y1 =  x * p + oldx  * p - k * y1;
        y2 = y1 * p + oldy1 * p - k * y2;
        y3 = y2 * p + oldy2 * p - k * y3;
        y4 = y3 * p + oldy3 * p - k * y4;
        
        // Clipping band-limited sigmoid
        y4 -= (y4 * y4 * y4) / 6.0;
        
        oldx = x;
        oldy1 = y1;
        oldy2 = y2;
        oldy3 = y3;
        
        return y4;
    }
};

MoogFilter lp_a[2];
MoogFilter lp_b[2];
MoogFilter lp_c[2];

FastLowpass fastlp_a[2];
FastHighpass fasthp_c[2];

struct GrooveApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = lab::MakeAudioContext();
        
        std::shared_ptr<FunctionNode> grooveBox;
        std::shared_ptr<GainNode> masterGain;
        std::shared_ptr<ADSRNode> envelope;
        
        float songLenSeconds = 16.0f;
        
        {
            ContextGraphLock g(context, "GrooveApp");
            ContextRenderLock r(context, "GrooveApp");
            
            float elapsedTime = 0.0f;
            
            envelope = std::make_shared<ADSRNode>(context->sampleRate());
            envelope->set(6.0f, 0.5f, 14.0f, 0.0f, songLenSeconds);
        
            float lfo_a, lfo_b, lfo_c;
            float bassWaveform, percussiveWaveform, bassSample;
            float padWaveform, padSample;
            float kickWaveform, kickSample;
            float synthWaveform, synthPercussive, synthDegradedWaveform, synthSample;
            
            grooveBox = std::make_shared<FunctionNode>(context->sampleRate(), 2);
            grooveBox->setFunction([&](ContextRenderLock& r, FunctionNode * self, int channel, float * samples, size_t framesToProcess)
            {
                double dt = 1.0 / self->sampleRate(); // time duration of one sample
                double now = self->now();
                
                int nextMeasure = int((now / 2)) % bassline.size();
                auto bm = bassline[nextMeasure];
                
                int nextNote =  int((now * 4)) % bm.size();
                float bn = note(bm[nextNote], 0);
                
                auto p = chords[int(now / 4) % chords.size()];
                
                auto mn = note(melody[int(now * 3) % melody.size()], int(2 - (now * 3)) % 4);

                for (size_t i = 0; i < framesToProcess; ++i)
                {
                    lfo_a = quickSin(2.0f, now);
                    lfo_b = quickSin(1.0f / 32.0f, now);
                    lfo_c = quickSin(1.0f / 128.0f, now);

                    // Bass
                    bassWaveform = quickSaw(bn, now) * 1.9f + quickSqr(bn / 2.f, now) * 1.0f + quickSin(bn / 2.f, now) * 2.2f + quickSqr(bn * 3.f, now) * 3.f;
                    percussiveWaveform = perc(bassWaveform / 3.f, 48.0f, fmod(now, 0.125f), now) * 1.0f;
                    bassSample = lp_a[channel].process(1000.f + (lfo_b * 140.f), quickSin(0.5f, now + 0.75f) * 0.2f, percussiveWaveform);
                   
                    // Pad
                    padWaveform = 5.1f * quickSaw(note(p[0], 1.f), now) + 3.9f * quickSaw(note(p[1], 2.f), now) + 4.0f * quickSaw(note(p[2], 1.f), now) + 3.0f * quickSqr(note(p[3], 0.0f), now);
                    padSample = 1.0f - ((quickSin(2.0f, now) * 0.28f) + 0.5f) * fasthp_c[channel](0.5f, lp_c[channel].process(1100.f + (lfo_a * 150.f), 0.05f, padWaveform * 0.03f));
                    
                    // Kick
                    kickWaveform = hardClip(0.37f, quickSin(note(7.0f, -1.f), now)) * 2.0f + hardClip(0.07f, quickSaw(note(7.03f,-1.0f), now * 0.2f)) * 4.00f;
                    kickSample = quickSaw(2.f, now) * 0.054f + fastlp_a[channel](240.0f, perc(hardClip(0.6f, kickWaveform), 54.f, fmod(now, 0.5f), now)) * 2.f;
                    
                    // Synth
                    synthWaveform = quickSaw(mn, now + 1.0f) + quickSqr(mn * 2.02f, now) * 0.4f + quickSqr(mn * 3.f, now + 2.f);
                    synthPercussive = lp_b[channel].process(3200.0f + (lfo_a * 400.f), 0.1f, perc(synthWaveform, 1.6f, fmod(now, 4.f), now) * 1.7f) * 1.8f;
                    synthDegradedWaveform = synthPercussive * quickSin(note(5.0f, 2.0f), now);
                    synthSample = 0.4f * synthPercussive + 0.05f * synthDegradedWaveform;
                    
                    // Mixer
                    samples[i] = (0.66 * hardClip(0.65f, bassSample)) + (0.50 * padSample) + (0.66 * synthSample) + (2.75 * kickSample);
                    
                    now += dt;
                }
                
                elapsedTime += now;
                
            });
            
            grooveBox->start(0);
            envelope->noteOn(0.0);
            
            grooveBox->connect(context.get(), envelope.get(), 0, 0);
            envelope->connect(context.get(), context->destination().get(), 0, 0);
        }
        
        int nowInSeconds = 0;
        while(nowInSeconds < songLenSeconds)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            nowInSeconds += 1;
        }
        
        lab::CleanupAudioContext(context);
    }
};
