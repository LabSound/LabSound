#include "ExampleBaseApp.h"
#include <cmath>
#include <algorithm>

// Unexpected Token from Wavepot. Shows the utility of LabSound as an experimental playground for DSP.

int transpose = 0;

// gets note 'n' frequency of 'octave'
float note(int n, int octave = 0)
{
    n += transpose;
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
    
    const float sampleRate = 44100.f;
    
    float process(double cutoff, double resonance, double sample)
    {
        cutoff = 2.0 * cutoff / sampleRate;
        
        p = cutoff * (1.8 - (0.8 * cutoff));
        k = 2.0 * std::sin(cutoff * (M_PI * 0.5)) - 1.0;
        t1 = (1.0 - p) * 1.386249;
        t2 = 12.0 + t1 * t1;
        r = resonance * (t2 + 6.0 * t1) / (t2 - 6.0 * t1);
        
        x = sample - r * y4;
        
        // four cascaded one-pole filters (bilinear transform)
        y1 =  x * p + oldx  * p - k * y1;
        y2 = y1 * p + oldy1 * p - k * y2;
        y3 = y2 * p + oldy2 * p - k * y3;
        y4 = y3 * p + oldy3 * p - k * y4;
        
        // clipper band limited sigmoid
        y4 -= (y4 * y4 * y4) / 6.0;
        
        oldx = x;
        oldy1 = y1;
        oldy2 = y2;
        oldy3 = y3;
        
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
        
        std::shared_ptr<FunctionNode> grooveBox;
        
        std::shared_ptr<GainNode> masterGain;
        
        {
            ContextGraphLock g(context, "GrooveApp");
            ContextRenderLock r(context, "GrooveApp");
            
            float elapsedTime = 0.0f;
            
            grooveBox = std::make_shared<FunctionNode>(context->sampleRate(), 1);
            grooveBox->setFunction([&elapsedTime](ContextRenderLock& r, FunctionNode * self, int channel, float * samples, size_t framesToProcess)
            {
                double dt = 1.0 / self->sampleRate(); // time duration of one sample
                
                double now = self->now();
                
                // bass
                int selection = int((now / 2)) % bassline.size();
                auto bm = bassline[selection];
                
                int selection2 =  int((now * 4)) % bm.size();
                float bn = note(bm[selection2], 0);
                
                for (size_t i = 0; i < framesToProcess; ++i)
                {
                    float lfo_a = quickSin(2.0f, now);
                    float lfo_b = quickSin(1.0f / 32.0f, now);
                    float lfo_c = quickSin(1.0f / 128.0f, now);
                    
                    float cutoff = 300 + (lfo_a * 60) + (lfo_b * 300) + (lfo_c * 250);
                    
                    float bassWaveform = quickSaw(bn, now) * 1.9f + quickSqr(bn / 2.f, now) * 1.0f + quickSin(bn / 2.f, now) * 2.2f + quickSqr(bn * 3.f, now) * 3.f;
                    
                    // This filter isn't quite working. Perc is fine, the lfos are fine. What's up with the distortion on the Moog?
                    float percSample = perc(bassWaveform / 3.f, 48.0f, fmod(now, 0.125f), now) * 1.0f;
                    float bassSample = lp_a.process(880, 0.15, percSample);
                    
                    //float testSamp = quickSqr(220, now);
                    //float testSampFilered = lp_b.process(300, 0.15, testSamp);
                    
                    samples[i] = bassSample; //hardClip(0.64f, bassSample);
                    
                    now += dt;
                }
                
                elapsedTime += now;
                
            });
            grooveBox->start(0);
            
            masterGain = std::make_shared<GainNode>(context->sampleRate());
            masterGain->gain()->setValue(0.5f);
            
            grooveBox->connect(context.get(), masterGain.get(), 0, 0);
            
            masterGain->connect(context.get(), context->destination().get(), 0, 0);
            
        }
        
        int now = 0;
        while(now < 10)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            now += 1;
        }
        
        LabSound::finish(context);
        
    }
};
