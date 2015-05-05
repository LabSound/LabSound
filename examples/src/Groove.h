#include "ExampleBaseApp.h"
#include <cmath>
#include <algorithm>

// Unexpected Token from Wavepot

int transpose = 0;

// gets note 'n' frequency of 'octave'
float note(int n, int octave = 0)
{
    n += transpose;
    return std::pow(2, (n - 33 + (12 * octave)) / 12) * 440;
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

struct FastLowpass
{
    float v = 0;
    float n;
    FastLowpass(float n) : n(n) {}
    float process(float input)
    {
        return v += (input - v) / n;
    }
};

struct FastHighpass
{
    float v = 0;
    float n;
    FastHighpass(float n) : n(n) {}
    float process(float input)
    {
      return v += input - v * n;
    }
};

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
    
    const float sampleRate = 44100;
    
    float process(float cutoff, float resonance, float sample)
    {
        cutoff = 2 * cutoff / sampleRate;
        
        p = cutoff * (1.8 - (0.8 * cutoff));
        k = 2 * std::sin(cutoff * M_PI * 0.5) - 1;
        t1 = (1 - p) * 1.386249;
        t2 = 12 + t1 * t1;
        r = resonance * (t2 + 6 * t1) / (t2 - 6 * t1);
        
        x = sample - r * y4;
        
        // four cascaded one-pole filters (bilinear transform)
        y1 =  x * p + oldx  * p - k * y1;
        y2 = y1 * p + oldy1 * p - k * y2;
        y3 = y2 * p + oldy2 * p - k * y3;
        y4 = y3 * p + oldy3 * p - k * y4;
        
        // clipper band limited sigmoid
        y4 -= (y4 * y4 * y4) / 6;
        
        oldx = x; oldy1 = y1; oldy2 = y2; oldy3 = y3;
        
        return y4;
    }
};

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

float perc(float wave, float decay, float o, float t)
{
    float env = std::max(0.f, 0.889f - (o * decay) / ((o * decay) + 1));
    return wave * env;
}

float perc_b(float wave, float decay, float o, float t)
{
    float env = std::min(0.f, 0.950f - (o * decay) / ((o * decay) + 1));
    return wave * env;
}

float hardClip(float n, float x)
{
    return x > n
    ? n
    : x < -n
    ? -n
    : x;
}

MoogFilter lp_a;
MoogFilter lp_b;
MoogFilter lp_c;

FastLowpass fastlp_a(240.0f);
FastLowpass fastlp_b(30.0f);

FastHighpass fasthp_a(1.7f);
FastHighpass fasthp_b(1.5f);
FastHighpass fasthp_c(0.5f);

struct GrooveApp : public LabSoundExampleApp
{
    void PlayExample()
    {
        auto context = LabSound::init();
        
        /*
        std::shared_ptr<FunctionNode> sweep;
        std::shared_ptr<FunctionNode> outputGainFunction;
        
        std::shared_ptr<OscillatorNode> osc;
        std::shared_ptr<GainNode> oscGain;
        std::shared_ptr<OscillatorNode> resonator;
        std::shared_ptr<GainNode> resonatorGain;
        std::shared_ptr<GainNode> resonanceSum;
        
        std::shared_ptr<DelayNode> delay[5];
        
        std::shared_ptr<GainNode> delaySum;
        std::shared_ptr<GainNode> filterSum;
        
        std::shared_ptr<BiquadFilterNode> filter[5];
        
        {
            ContextGraphLock g(context, "Red Alert");
            ContextRenderLock r(context, "Red Alert");
            
            sweep = std::make_shared<FunctionNode>(context->sampleRate(), 1);
            sweep->setFunction([](ContextRenderLock& r, FunctionNode *self, int channel, float* values, size_t framesToProcess) {
                double dt = 1.0 / self->sampleRate();
                double now = fmod(self->now(), 1.2);
                
                for (size_t i = 0; i < framesToProcess; ++i) {
                    //0 to 1 in 900 ms with a 1200ms gap in between
                    if (now > 0.9)
                        values[i] = 487. + 360.;
                    else {
                        values[i] = sqrt(now * 1.f/0.9f) * 487. + 360.;
                    }
                    
                    now += dt;
                }
            });
            sweep->start(0);
            
            outputGainFunction = std::make_shared<FunctionNode>(context->sampleRate(), 1);
            outputGainFunction->setFunction([](ContextRenderLock& r, FunctionNode *self, int channel, float* values, size_t framesToProcess) {
                double dt = 1.0 / self->sampleRate();
                double now = fmod(self->now(), 1.2);
                
                for (size_t i = 0; i < framesToProcess; ++i) {
                    //0 to 1 in 900 ms with a 1200ms gap in between
                    if (now > 0.9)
                        values[i] = 0;
                    else {
                        values[i] = 0.2f;
                    }
                    
                    now += dt;
                }
            });
            outputGainFunction->start(0);

            osc = std::make_shared<OscillatorNode>(r, context->sampleRate());
            osc->setType(r, OscillatorType::SAWTOOTH);
            osc->frequency()->setValue(220);
            osc->start(0);
            oscGain = std::make_shared<GainNode>(context->sampleRate());
            oscGain->gain()->setValue(0.5f);
            
            resonator = std::make_shared<OscillatorNode>(r, context->sampleRate());
            resonator->setType(r, OscillatorType::SINE);
            resonator->frequency()->setValue(220);
            resonator->start(0);
            
            resonatorGain = std::make_shared<GainNode>(context->sampleRate());
            resonatorGain->gain()->setValue(0.0f);

            resonanceSum = std::make_shared<GainNode>(context->sampleRate());
            resonanceSum->gain()->setValue(0.5f);
            
            // sweep drives oscillator frequency
            sweep->connect(g, osc->frequency(), 0);
            
            // oscillator drives resonator frequency
            osc->connect(g, resonator->frequency(), 0);

            // osc --> oscGain -------------+
            // resonator -> resonatorGain --+--> resonanceSum
            //
            osc->connect(context.get(), oscGain.get(), 0, 0);
            oscGain->connect(context.get(), resonanceSum.get(), 0, 0);
            resonator->connect(context.get(), resonatorGain.get(), 0, 0);
            resonatorGain->connect(context.get(), resonanceSum.get(), 0, 0);
            
            delaySum = std::make_shared<GainNode>(context->sampleRate());
            delaySum->gain()->setValue(0.2f);
            
            // resonanceSum --+--> delay0 --+
            //                +--> delay1 --+
            //                + ...    .. --+
            //                +--> delay4 --+---> delaySum
            //
            float delays[5] = {0.015, 0.022, 0.035, 0.024, 0.011};
            for (int i = 0; i < 5; ++i) {
                delay[i] = std::make_shared<DelayNode>(context->sampleRate(), 0.04f);
                delay[i]->delayTime()->setValue(delays[i]);
                resonanceSum->connect(context.get(), delay[i].get(), 0, 0);
                delay[i]->connect(context.get(), delaySum.get(), 0, 0);
            }
            
            filterSum = std::make_shared<GainNode>(context->sampleRate());
            filterSum->gain()->setValue(0.2f);
            
            // delaySum --+--> filter0 --+
            //            +--> filter1 --+
            //            +--> filter2 --+
            //            +--> filter3 --+
            //            +--------------+----> filterSum
            //
            delaySum->connect(context.get(), filterSum.get(), 0, 0);

            float centerFrequencies[4] = {740.f, 1400.f, 1500.f, 1600.f};
            for (int i = 0; i < 4; ++i) {
                filter[i] = std::make_shared<BiquadFilterNode>(context->sampleRate());
                filter[i]->frequency()->setValue(centerFrequencies[i]);
                filter[i]->q()->setValue(12.f);
                delaySum->connect(context.get(), filter[i].get(), 0, 0);
                filter[i]->connect(context.get(), filterSum.get(), 0, 0);
            }
            
            // filterSum --> destination
            //
            outputGainFunction->connect(g, filterSum->gain(), 0);
            filterSum->connect(context.get(), context->destination().get(), 0, 0);
        }
        

        
         */
        
        int now = 0.0;
        while(now < 10000)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            now += 1000;
        }
        
        LabSound::finish(context);
        
    }
};
