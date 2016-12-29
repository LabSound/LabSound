// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.
// This files contains portions of sfxr. The original code contained the following notes:

/*
 (http://www.drpetter.se/project_sfxr.html)

 -----------------------------
 sfxr - sound effect generator
 -----------------------------
 by DrPetter, 2007-12-14
 developed for LD48#10
 -----------------------------


 Basic usage:

 Start the application, then hit
 some of the buttons on the left
 side to generate random sounds
 matching the button descriptions.

 Press "Export .WAV" to save the
 current sound as a WAV audio file.
 Click the buttons below to change
 WAV format in terms of bits per
 sample and sample rate.

 If you find a sound that is sort
 of interesting but not quite what
 you want, you can drag some sliders
 around until it sounds better.

 The Randomize button generates
 something completely random.

 Mutate slightly alters the current
 parameters to automatically create
 a variation of the sound.



 Advanced usage:

 Figure out what each slider does and
 use them to adjust particular aspects
 of the current sound...

 Press the right mouse button on a slider
 to reset it to a value of zero.

 Press Space or Enter to play the current sound.

 The Save/Load sound buttons allow saving
 and loading of program parameters to work
 on a sound over several sessions.

 Volume setting is saved with the sound and
 exported to WAV. If you increase it too much
 there's a risk of clipping.

 Some parameters influence the sound during
 playback (particularly when using a non-zero
 repeat speed), and dragging these sliders
 can cause some interesting effects.
 To record this you will need to use an external
 recording application, for instance Audacity.
 Set the recording source in that application
 to "Wave", "Stereo Mix", "Mixed Output" or similar.

 Using an external sound editor to capture and edit
 sound can also be used to string several sounds
 together for more complex results.

 Parameter description:
 - The top four buttons select base waveform
 - First four parameters control the volume envelope
 Attack is the beginning of the sound,
 longer attack means a smoother start.
 Sustain is how long the volume is held constant
 before fading out.
 Increase Sustain Punch to cause a popping
 effect with increased (and falling) volume
 during the sustain phase.
 Decay is the fade-out time.
 - Next six are for controlling the sound pitch or
 frequency.
 Start frequency is pretty obvious. Has a large
 impact on the overall sound.
 Min frequency represents a cutoff that stops all
 sound if it's passed during a downward slide.
 Slide sets the speed at which the frequency should
 be swept (up or down).
 Delta slide is the "slide of slide", or rate of change
 in the slide speed.
 Vibrato depth/speed makes for an oscillating
 frequency effect at various strengths and rates.
 - Then we have two parameters for causing an abrupt
 change in pitch after a ceratin delay.
 Amount is pitch change (up or down)
 and Speed indicates time to wait before changing
 the pitch.
 - Following those are two parameters specific to the
 squarewave waveform.
 The duty cycle of a square describes its shape
 in terms of how large the positive vs negative
 sections are. It can be swept up or down by
 changing the second parameter.
 - Repeat speed, when not zero, causes the frequency
 and duty parameters to be reset at regular intervals
 while the envelope and filter continue unhindered.
 This can make for some interesting pulsating effects.
 - Phaser offset overlays a delayed copy of the audio
 stream on top of itself, resulting in a kind of tight
 reverb or sci-fi effect.
 This parameter can also be swept like many others.
 - Finally, the bottom five sliders control two filters
 which are applied after all other effects.
 The first one is a resonant lowpass filter which has
 a sweepable cutoff frequency.
 The other is a highpass filter which can be used to
 remove undesired low frequency hum in "light" sounds.


 ----------------------


 License
 -------

 Basically, I don't care what you do with it, anything goes.

 To please all the troublesome folks who request a formal license,
 I attach the "MIT license" as follows:

 --

 Copyright (c) 2007 Tomas Pettersson

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.



 ----------------------

 http://www.drpetter.se

 drpetter@gmail.com

 ----------------------

*/

#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/SfxrNode.h"

#include "internal/AudioBus.h"

#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

using namespace std;
using namespace lab;
using namespace lab;

inline uint32_t rnd(uint32_t n)
{
    if (n == 1)
        return ((rand() & 0x7fff) > 0x3fff) ? 1 : 0;
    return rand() % (n+1);
}

#define PI 3.14159265f

inline float frnd(float range)
{
    return (float)rnd(10000)/10000*range;
}

inline float rndr(float from, float to) {
    return frnd(1) * (to - from) + from;
}

inline float sqr(float a) { return a * a; }
inline float cube(float a) { return a * a * a; }
inline float flurp(float a) { return a / (1.0f - a); }


class SfxrNode::Sfxr {
public:

    int wave_type;

    float p_base_freq;
    float p_freq_limit;
    float p_freq_ramp;
    float p_freq_dramp;
    float p_duty;
    float p_duty_ramp;

    float p_vib_strength;
    float p_vib_speed;
    float p_vib_delay;

    float p_env_attack;
    float p_env_sustain;
    float p_env_decay;
    float p_env_punch;

    bool filter_on;
    float p_lpf_resonance;
    float p_lpf_freq;
    float p_lpf_ramp;
    float p_hpf_freq;
    float p_hpf_ramp;

    float p_pha_offset;
    float p_pha_ramp;

    float p_repeat_speed;

    float p_arp_speed;
    float p_arp_mod;

    float master_vol;

    float sound_vol;


    bool playing_sample;
    int phase;
    double fperiod;
    double fmaxperiod;
    double fslide;
    double fdslide;
    int period;
    float square_duty;
    float square_slide;
    int env_stage;
    int env_time;
    int env_length[3];
    float env_vol;
    float fphase;
    float fdphase;
    int iphase;
    float phaser_buffer[1024];
    int ipp;
    float noise_buffer[32];
    float fltp;
    float fltdp;
    float fltw;
    float fltw_d;
    float fltdmp;
    float fltphp;
    float flthp;
    float flthp_d;
    float vib_phase;
    float vib_speed;
    float vib_amp;
    int rep_time;
    int rep_limit;
    int arp_time;
    int arp_limit;
    double arp_mod;

    int wav_bits;
    int wav_freq;

    int file_sampleswritten;
    float filesample;
    int fileacc;

    void ResetParams();
    void ResetSample(bool restart);
    void PlaySample();
    void SynthSample(int length, float* buffer, FILE* file);
};

void SfxrNode::Sfxr::ResetParams()
{
    wave_type=0;

    p_base_freq=0.3f;
    p_freq_limit=0.0f;
    p_freq_ramp=0.0f;
    p_freq_dramp=0.0f;
    p_duty=0.0f;
    p_duty_ramp=0.0f;

    p_vib_strength=0.0f;
    p_vib_speed=0.0f;
    p_vib_delay=0.0f;

    p_env_attack=0.0f;
    p_env_sustain=0.3f;
    p_env_decay=0.4f;
    p_env_punch=0.0f;

    filter_on=false;
    p_lpf_resonance=0.0f;
    p_lpf_freq=1.0f;
    p_lpf_ramp=0.0f;
    p_hpf_freq=0.0f;
    p_hpf_ramp=0.0f;

    p_pha_offset=0.0f;
    p_pha_ramp=0.0f;

    p_repeat_speed=0.0f;

    p_arp_speed=0.0f;
    p_arp_mod=0.0f;

    wav_bits = 16;
    wav_freq = 44100;
    sound_vol = 0.5f;
    master_vol = 0.05f;
}


void SfxrNode::Sfxr::ResetSample(bool restart)
{
    if(!restart)
        phase=0;
    fperiod=100.0/(p_base_freq*p_base_freq+0.001);
    period=(int)fperiod;
    fmaxperiod=100.0/(p_freq_limit*p_freq_limit+0.001);
    fslide=1.0-pow((double)p_freq_ramp, 3.0)*0.01;
    fdslide=-pow((double)p_freq_dramp, 3.0)*0.000001;
    square_duty=0.5f-p_duty*0.5f;
    square_slide=-p_duty_ramp*0.00005f;
    if(p_arp_mod>=0.0f)
        arp_mod=1.0-pow((double)p_arp_mod, 2.0)*0.9;
    else
        arp_mod=1.0+pow((double)p_arp_mod, 2.0)*10.0;
    arp_time=0;
    arp_limit=(int)(pow(1.0f-p_arp_speed, 2.0f)*20000+32);
    if(p_arp_speed==1.0f)
        arp_limit=0;
    if(!restart)
    {
        // reset filter
        fltp=0.0f;
        fltdp=0.0f;
        fltw=pow(p_lpf_freq, 3.0f)*0.1f;
        fltw_d=1.0f+p_lpf_ramp*0.0001f;
        fltdmp=5.0f/(1.0f+pow(p_lpf_resonance, 2.0f)*20.0f)*(0.01f+fltw);
        if(fltdmp>0.8f) fltdmp=0.8f;
        fltphp=0.0f;
        flthp=pow(p_hpf_freq, 2.0f)*0.1f;
        flthp_d=1.0+p_hpf_ramp*0.0003f;
        // reset vibrato
        vib_phase=0.0f;
        vib_speed=pow(p_vib_speed, 2.0f)*0.01f;
        vib_amp=p_vib_strength*0.5f;
        // reset envelope
        env_vol=0.0f;
        env_stage=0;
        env_time=0;
        env_length[0]=(int)(p_env_attack*p_env_attack*100000.0f);
        env_length[1]=(int)(p_env_sustain*p_env_sustain*100000.0f);
        env_length[2]=(int)(p_env_decay*p_env_decay*100000.0f);

        fphase=pow(p_pha_offset, 2.0f)*1020.0f;
        if(p_pha_offset<0.0f) fphase=-fphase;
        fdphase=pow(p_pha_ramp, 2.0f)*1.0f;
        if(p_pha_ramp<0.0f) fdphase=-fdphase;
        iphase=abs((int)fphase);
        ipp=0;
        for(int i=0;i<1024;i++)
            phaser_buffer[i]=0.0f;

        for(int i=0;i<32;i++)
            noise_buffer[i]=frnd(2.0f)-1.0f;

        rep_time=0;
        rep_limit=(int)(pow(1.0f-p_repeat_speed, 2.0f)*20000+32);
        if(p_repeat_speed==0.0f)
            rep_limit=0;
    }
}

void SfxrNode::Sfxr::PlaySample()
{
    ResetSample(false);
    playing_sample=true;
}

void SfxrNode::Sfxr::SynthSample(int length, float* buffer, FILE* file)
{
    for(int i=0;i<length;i++)
    {
        if(!playing_sample)
            break;

        rep_time++;
        if(rep_limit!=0 && rep_time>=rep_limit)
        {
            rep_time=0;
            ResetSample(true);
        }

        // frequency envelopes/arpeggios
        arp_time++;
        if(arp_limit!=0 && arp_time>=arp_limit)
        {
            arp_limit=0;
            fperiod*=arp_mod;
        }
        fslide+=fdslide;
        fperiod*=fslide;
        if(fperiod>fmaxperiod)
        {
            fperiod=fmaxperiod;
            if(p_freq_limit>0.0f)
                playing_sample=false;
        }
        float rfperiod=fperiod;
        if(vib_amp>0.0f)
        {
            vib_phase+=vib_speed;
            rfperiod=fperiod*(1.0+sin(vib_phase)*vib_amp);
        }
        period=(int)rfperiod;
        if(period<8) period=8;
        square_duty+=square_slide;
        if(square_duty<0.0f) square_duty=0.0f;
        if(square_duty>0.5f) square_duty=0.5f;
        // volume envelope
        env_time++;
        if(env_time>env_length[env_stage])
        {
            env_time=0;
            env_stage++;
            if(env_stage==3)
                playing_sample=false;
        }
        if(env_stage==0)
            env_vol=(float)env_time/env_length[0];
        if(env_stage==1)
            env_vol=1.0f+pow(1.0f-(float)env_time/env_length[1], 1.0f)*2.0f*p_env_punch;
        if(env_stage==2)
            env_vol=1.0f-(float)env_time/env_length[2];

        // phaser step
        fphase+=fdphase;
        iphase=abs((int)fphase);
        if(iphase>1023) iphase=1023;

        if(flthp_d!=0.0f)
        {
            flthp*=flthp_d;
            if(flthp<0.00001f) flthp=0.00001f;
            if(flthp>0.1f) flthp=0.1f;
        }

        float ssample=0.0f;
        for(int si=0;si<8;si++) // 8x supersampling
        {
            float sample=0.0f;
            phase++;
            if(phase>=period)
            {
                //                phase=0;
                phase%=period;
                if(wave_type==NOISE)
                    for(int i=0;i<32;i++)
                        noise_buffer[i]=frnd(2.0f)-1.0f;
            }
            // base waveform
            float fp=(float)phase/period;
            switch(wave_type)
            {
                case 0: // square
                    if(fp<square_duty)
                        sample=0.5f;
                    else
                        sample=-0.5f;
                    break;
                case 1: // sawtooth
                    sample=1.0f-fp*2;
                    break;
                case 2: // sine
                    sample=(float)sin(fp*2*PI);
                    break;
                case 3: // noise
                    sample=noise_buffer[phase*32/period];
                    break;
            }
            // lp filter
            float pp=fltp;
            fltw*=fltw_d;
            if(fltw<0.0f) fltw=0.0f;
            if(fltw>0.1f) fltw=0.1f;
            if(p_lpf_freq!=1.0f)
            {
                fltdp+=(sample-fltp)*fltw;
                fltdp-=fltdp*fltdmp;
            }
            else
            {
                fltp=sample;
                fltdp=0.0f;
            }
            fltp+=fltdp;
            // hp filter
            fltphp+=fltp-pp;
            fltphp-=fltphp*flthp;
            sample=fltphp;
            // phaser
            phaser_buffer[ipp&1023]=sample;
            sample+=phaser_buffer[(ipp-iphase+1024)&1023];
            ipp=(ipp+1)&1023;
            // final accumulation and envelope application
            ssample+=sample*env_vol;
        }
        ssample=ssample/8*master_vol;

        ssample*=2.0f*sound_vol;

        if(buffer!=NULL)
        {
            if(ssample>1.0f) ssample=1.0f;
            if(ssample<-1.0f) ssample=-1.0f;
            *buffer++=ssample;
        }
        if(file!=NULL)
        {
            // quantize depending on format
            // accumulate/count to accomodate variable sample rate?
            ssample*=4.0f; // arbitrary gain to get reasonable output volume...
            if(ssample>1.0f) ssample=1.0f;
            if(ssample<-1.0f) ssample=-1.0f;
            filesample+=ssample;
            fileacc++;
            if(wav_freq==44100 || fileacc==2)
            {
                filesample/=fileacc;
                fileacc=0;
                if(wav_bits==16)
                {
                    short isample=(short)(filesample*32000);
                    fwrite(&isample, 1, 2, file);
                }
                else
                {
                    unsigned char isample=(unsigned char)(filesample*127+128);
                    fwrite(&isample, 1, 1, file);
                }
                filesample=0.0f;
            }
            file_sampleswritten++;
        }
    }
}

#pragma mark _______________________
#pragma mark Node Interface

namespace lab {

    SfxrNode::SfxrNode(float sampleRate)
    : AudioScheduledSourceNode(sampleRate)
    , sfxr(new SfxrNode::Sfxr())
    {
        setNodeType(lab::NodeType::NodeTypeSfxr);

        // Output is always mono.
        addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

        _waveType = make_shared<AudioParam>("waveType", 0, 0, 3);
        _attack = make_shared<AudioParam>("attack", 0, 0, 1);
        _sustainTime = make_shared<AudioParam>("sustain", 0.3, 0, 1);
        _sustainPunch = make_shared<AudioParam>("sustainPunch", 0, 0, 1);
        _startFrequency = make_shared<AudioParam>("startFrequency", 0.3, 0, 1);
        _minFrequency = make_shared<AudioParam>("minFrequency", 0, 0, 1);
        _decayTime = make_shared<AudioParam>("decay", 0.4f, 0, 1);
        _slide = make_shared<AudioParam>("slide", 0, -1, 1);
        _deltaSlide = make_shared<AudioParam>("deltaSlide", 0, 0, 1);
        _vibratoDepth = make_shared<AudioParam>("vibratoDepth", 0, 0, 1);
        _vibratoSpeed = make_shared<AudioParam>("vibratoSpeed", 0, 0, 1);
        _changeAmount = make_shared<AudioParam>("changeAmount", 0, -1, 1);
        _changeSpeed = make_shared<AudioParam>("changeSpeed", 0, 0, 1);
        _squareDuty = make_shared<AudioParam>("squareDuty", 0, 0, 1);
        _dutySweep = make_shared<AudioParam>("dutySweep", 0, -1, 1);
        _repeatSpeed = make_shared<AudioParam>("repeatSpeed", 0, 0, 1);
        _phaserOffset = make_shared<AudioParam>("phaserOffset", 0, -1, 1);
        _phaserSweep = make_shared<AudioParam>("phaserSweep", 0, -1, 1);
        _lpFilterCutoff = make_shared<AudioParam>("lpFiterCutoff", 1.0f, 0, 1);
        _lpFilterCutoffSweep = make_shared<AudioParam>("lpFilterCutoffSweep", 0, -1, 1);
        _lpFiterResonance = make_shared<AudioParam>("lpFiterResonance", 0, 0, 1);
        _hpFilterCutoff = make_shared<AudioParam>("hpFiterCutoff", 0, 0, 1);
        _hpFilterCutoffSweep = make_shared<AudioParam>("hpFilterCutoffSweep", 0, -1, 1);

        m_params.push_back(_waveType);
        m_params.push_back(_attack);
        m_params.push_back(_sustainTime);
        m_params.push_back(_sustainPunch);
        m_params.push_back(_startFrequency);
        m_params.push_back(_minFrequency);
        m_params.push_back(_decayTime);
        m_params.push_back(_slide);
        m_params.push_back(_deltaSlide);
        m_params.push_back(_vibratoDepth);
        m_params.push_back(_vibratoSpeed);
        m_params.push_back(_changeAmount);
        m_params.push_back(_changeSpeed);
        m_params.push_back(_squareDuty);
        m_params.push_back(_dutySweep);
        m_params.push_back(_repeatSpeed);
        m_params.push_back(_phaserOffset);
        m_params.push_back(_phaserSweep);
        m_params.push_back(_lpFilterCutoff);
        m_params.push_back(_lpFilterCutoffSweep);
        m_params.push_back(_lpFiterResonance);
        m_params.push_back(_hpFilterCutoff);
        m_params.push_back(_hpFilterCutoffSweep);

        sfxr->ResetParams();
        sfxr->ResetSample(true);
        sfxr->PlaySample();

        initialize();
    }

    SfxrNode::~SfxrNode()
    {
        uninitialize();
    }

    void SfxrNode::noteOn() {
        start(0);
        sfxr->ResetSample(true);
        sfxr->ResetSample(false);
        sfxr->PlaySample();
    }

    void SfxrNode::process(ContextRenderLock& r, size_t framesToProcess)
    {
        AudioBus* outputBus = output(0)->bus(r);

        if (!isInitialized() || !outputBus->numberOfChannels()) {
            outputBus->zero();
            return;
        }

        size_t quantumFrameOffset;
        size_t nonSilentFramesToProcess;

        updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);

        if (!nonSilentFramesToProcess) {
            outputBus->zero();
            return;
        }

        float* destP = outputBus->channel(0)->mutableData();

        // Start rendering at the correct offset.
        destP += quantumFrameOffset;
        int n = nonSilentFramesToProcess;

#define UPDATE(typ, cur, val) \
{ typ v = val->value(r); if (sfxr->cur != v) { needUpdate = true; sfxr->cur = v;} }

        bool needUpdate = false;
        UPDATE(int, wave_type, _waveType)
        UPDATE(float, p_base_freq, _startFrequency)
        UPDATE(float, p_freq_limit, _minFrequency)
        UPDATE(float, p_freq_ramp, _slide)
        UPDATE(float, p_freq_dramp, _deltaSlide)
        UPDATE(float, p_duty, _squareDuty)
        UPDATE(float, p_duty_ramp, _dutySweep)

        UPDATE(float, p_vib_strength, _vibratoDepth)
        UPDATE(float, p_vib_speed, _vibratoSpeed)

        UPDATE(float, p_env_attack, _attack)
        UPDATE(float, p_env_sustain, _sustainTime)
        UPDATE(float, p_env_decay, _decayTime)
        UPDATE(float, p_env_punch, _sustainPunch)

        UPDATE(float, p_lpf_resonance, _lpFiterResonance)
        sfxr->filter_on = sfxr->p_lpf_resonance > 0;
        UPDATE(float, p_lpf_freq, _lpFilterCutoff)
        UPDATE(float, p_lpf_ramp, _lpFilterCutoffSweep)
        UPDATE(float, p_hpf_freq, _hpFilterCutoff)
        UPDATE(float, p_hpf_ramp, _hpFilterCutoffSweep)

        UPDATE(float, p_pha_offset, _phaserOffset)
        UPDATE(float, p_pha_ramp, _phaserSweep)

        UPDATE(float, p_repeat_speed, _repeatSpeed)
        UPDATE(float, p_arp_speed, _changeSpeed)
        UPDATE(float, p_arp_mod, _changeAmount)

        if (needUpdate)
            sfxr->ResetSample(false);

#undef UPDATE

        float* fbuf = (float*) alloca(sizeof(float) * n);
        memset(fbuf, 0, sizeof(float) * n);
        sfxr->SynthSample(n, fbuf, NULL);
        while (n--)
        {
            float f = fbuf[n];
            if (f < -1.0f) f = -1.0f;
            else if (f > 1.0f) f = 1.0f;
            *destP++ = f;
        }

        outputBus->clearSilentFlag();
    }

    void SfxrNode::reset(ContextRenderLock&)
    {
    }

    bool SfxrNode::propagatesSilence(double now) const
    {
        return !isPlayingOrScheduled() || hasFinished();
    }


#pragma mark ____________________________________
#pragma mark Some default sounds

    // parameters for default sounds found here -
    // https://github.com/grumdrig/jsfxr/blob/master/sfxr.js

    void SfxrNode::setDefaultBeep() {
        // Wave shape
        _waveType->setValue(SQUARE);

        // Envelope
        _attack->setValue(0);
        _sustainTime->setValue(0.3);
        _sustainPunch->setValue(0);
        _decayTime->setValue(0.4);

        // Tone
        _startFrequency->setValue(0.3);
        _minFrequency->setValue(0);
        _slide->setValue(0);
        _deltaSlide->setValue(0);

        // Vibrato
        _vibratoDepth->setValue(0);
        _vibratoSpeed->setValue(0);

        // Tonal change
        _changeAmount->setValue(0);
        _changeSpeed->setValue(0);

        // Square wave duty (proportion of time signal is high vs. low)
        _squareDuty->setValue(0);
        _dutySweep->setValue(0);

        // Repeat
        _repeatSpeed->setValue(0);

        // Flanger
        _phaserOffset->setValue(0);
        _phaserSweep->setValue(0);

        // Low-pass filter
        _lpFilterCutoff->setValue(1);
        _lpFilterCutoffSweep->setValue(0);
        _lpFiterResonance->setValue(0);
        _hpFilterCutoff->setValue(0);
        _hpFilterCutoffSweep->setValue(0);

        // Sample parameters
        sfxr->sound_vol = 0.5;
        sfxr->wav_freq = 44100;
        sfxr->wav_bits = 16;
    }

    void SfxrNode::coin() {
        setDefaultBeep();
        _startFrequency->setValue(0.4f + frnd(0.5f));
        _attack->setValue(0);
        _sustainTime->setValue(0.1f);
        _decayTime->setValue(0.1f + frnd(0.4f));
        _sustainPunch->setValue(0.3f + frnd(0.3f));
        if (rnd(1)) {
            _changeSpeed->setValue(0.5f + frnd(0.2f));
            _changeAmount->setValue(0.2f + frnd(0.4f));
        }
    }

    /// @TODO this should be queued up for the next processing call to avoid the context lock requirement
    /// or we need a variation of value that is threadsafe
    void SfxrNode::laser(ContextRenderLock& r) {
        setDefaultBeep();
        _waveType->setValue(rnd(2));
        if(_waveType->value(r) == SINE && rnd(1))
            _waveType->setValue(rnd(1));
        if (rnd(2) == 0) {
            _startFrequency->setValue(0.3 + frnd(0.6));
            _minFrequency->setValue(frnd(0.1));
            _slide->setValue(-0.35 - frnd(0.3));
        } else {
            _startFrequency->setValue(0.5 + frnd(0.5));
            _minFrequency->setValue(_startFrequency->value(r) - 0.2 - frnd(0.6));
            if (_minFrequency->value(r) < 0.2) _minFrequency->setValue(0.2);
            _slide->setValue(-0.15 - frnd(0.2));
        }
        if (_waveType->value(r) == SAWTOOTH)
            _squareDuty->setValue(1);
        if (rnd(1)) {
            _squareDuty->setValue(frnd(0.5));
            _dutySweep->setValue(frnd(0.2));
        } else {
            _squareDuty->setValue(0.4 + frnd(0.5));
            _dutySweep->setValue(-frnd(0.7));
        }
        _attack->setValue(0);
        _sustainTime->setValue(0.1 + frnd(0.2));
        _decayTime->setValue(frnd(0.4));
        if (rnd(1))
            _sustainPunch->setValue(frnd(0.3));
        if (rnd(2) == 0) {
            _phaserOffset->setValue(frnd(0.2));
            _phaserSweep->setValue(-frnd(0.2));
        }
        _hpFilterCutoff->setValue(frnd(0.3));
    }

    void SfxrNode::explosion() {
        setDefaultBeep();
        _waveType->setValue(NOISE);
        if (rnd(1)) {
            _startFrequency->setValue(sqr(0.1 + frnd(0.4)));
            _slide->setValue(-0.1 + frnd(0.4));
        } else {
            _startFrequency->setValue(sqr(0.2 + frnd(0.7)));
            _slide->setValue(-0.2 - frnd(0.2));
        }
        if (rnd(4) == 0)
            _slide->setValue(0);
        if (rnd(2) == 0)
            _repeatSpeed->setValue(0.3 + frnd(0.5));
        _attack->setValue(0);
        _sustainTime->setValue(0.1 + frnd(0.3));
        _decayTime->setValue(frnd(0.5));
        if (rnd(1)) {
            _phaserOffset->setValue(-0.3 + frnd(0.9));
            _phaserSweep->setValue(-frnd(0.3));
        }
        _sustainPunch->setValue(0.2 + frnd(0.6));
        if (rnd(1)) {
            _vibratoDepth->setValue(frnd(0.7));
            _vibratoSpeed->setValue(frnd(0.6));
        }
        if (rnd(2) == 0) {
            _changeSpeed->setValue(0.6 + frnd(0.3));
            _changeAmount->setValue(0.8 - frnd(1.6));
        }
    }

    void SfxrNode::powerUp() {
        setDefaultBeep();
        if (rnd(1)) {
            _waveType->setValue(SAWTOOTH);
            _squareDuty->setValue(1);
        }
        else {
            _squareDuty->setValue(frnd(0.6));
        }
        _startFrequency->setValue(0.2 + frnd(0.3));
        if (rnd(1)) {
            _slide->setValue(0.1 + frnd(0.4));
            _repeatSpeed->setValue(0.4 + frnd(0.4));
        }
        else {
            _slide->setValue(0.05 + frnd(0.2));
            if (rnd(1)) {
                _vibratoDepth->setValue(frnd(0.7));
                _vibratoSpeed->setValue(frnd(0.6));
            }
        }
        _attack->setValue(0);
        _sustainTime->setValue(frnd(0.4));
        _decayTime->setValue(0.1 + frnd(0.4));
    }

    /// @TODO remove need for context lock see above
    void SfxrNode::hit(ContextRenderLock& r) {
        setDefaultBeep();
        _waveType->setValue(rnd(2));
        if (_waveType->value(r) == SINE)
            _waveType->setValue(NOISE);
        if (_waveType->value(r) == SQUARE)
            _squareDuty->setValue(frnd(0.6));
        if (_waveType->value(r) == SAWTOOTH)
            _squareDuty->setValue(1);
        _startFrequency->setValue(0.2 + frnd(0.6));
        _slide->setValue(-0.3 - frnd(0.4));
        _attack->setValue(0);
        _sustainTime->setValue(frnd(0.1));
        _decayTime->setValue(0.1 + frnd(0.2));
        if (rnd(1))
            _hpFilterCutoff->setValue(frnd(0.3));
    }

    void SfxrNode::jump() {
        setDefaultBeep();
        _waveType->setValue(SQUARE);
        _squareDuty->setValue(frnd(0.6));
        _startFrequency->setValue(0.3 + frnd(0.3));
        _slide->setValue(0.1 + frnd(0.2));
        _attack->setValue(0);
        _sustainTime->setValue(0.1 + frnd(0.3));
        _decayTime->setValue(0.1 + frnd(0.2));
        if (rnd(1))
            _hpFilterCutoff->setValue(frnd(0.3));
        if (rnd(1))
            _lpFilterCutoff->setValue(1 - frnd(0.6));
    }

    /// @TODO remove need for context lock see above
    void SfxrNode::select(ContextRenderLock& r) {
        setDefaultBeep();
        _waveType->setValue(rnd(1));
        if (_waveType->value(r) == SQUARE)
            _squareDuty->setValue(frnd(0.6));
        else
            _squareDuty->setValue(1);
        _startFrequency->setValue(0.2 + frnd(0.4));
        _attack->setValue(0);
        _sustainTime->setValue(0.1 + frnd(0.1));
        _decayTime->setValue(frnd(0.2));
        _hpFilterCutoff->setValue(0.1);
    }

    /// @TODO remove need for context lock see above
    void SfxrNode::mutate(ContextRenderLock& r) {
        if (rnd(1)) _startFrequency->setValue(_startFrequency->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _slide->setValue(_slide->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _deltaSlide->setValue(_deltaSlide->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _squareDuty->setValue(_squareDuty->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _dutySweep->setValue(_dutySweep->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _vibratoDepth->setValue(_vibratoDepth->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _vibratoSpeed->setValue(_vibratoSpeed->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _attack->setValue(_attack->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _sustainTime->setValue(_sustainTime->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _decayTime->setValue(_decayTime->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _sustainPunch->setValue(_sustainPunch->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _lpFiterResonance->setValue(_lpFiterResonance->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _lpFilterCutoff->setValue(_lpFilterCutoff->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _lpFilterCutoffSweep->setValue(_lpFilterCutoffSweep->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _hpFilterCutoff->setValue(_hpFilterCutoff->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _hpFilterCutoffSweep->setValue(_hpFilterCutoffSweep->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _phaserOffset->setValue(_phaserOffset->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _phaserSweep->setValue(_phaserSweep->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _repeatSpeed->setValue(_repeatSpeed->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _changeSpeed->setValue(_changeSpeed->value(r) + frnd(0.1) - 0.05);
        if (rnd(1)) _changeAmount->setValue(_changeAmount->value(r) + frnd(0.1) - 0.05);
    }

    /// @TODO remove need for context lock see above
    void SfxrNode::randomize(ContextRenderLock& r) {
        if (rnd(1))
            _startFrequency->setValue(cube(frnd(2) - 1) + 0.5);
        else
            _startFrequency->setValue(sqr(frnd(1)));
        _minFrequency->setValue(0);
        _slide->setValue(powf(frnd(2) - 1, 5));
        if (_startFrequency->value(r) > 0.7 && _slide->value(r) > 0.2)
            _slide->setValue(-_slide->value(r));
        if (_startFrequency->value(r) < 0.2 && _slide->value(r) < -0.05)
            _slide->setValue(-_slide->value(r));
        _deltaSlide->setValue(powf(frnd(2) - 1, 3));
        _squareDuty->setValue(frnd(2) - 1);
        _dutySweep->setValue(powf(frnd(2) - 1, 3));
        _vibratoDepth->setValue(powf(frnd(2) - 1, 3));
        _vibratoSpeed->setValue(rndr(-1, 1));
        _attack->setValue(cube(rndr(0, 1)));
        _sustainTime->setValue(sqr(rndr(0, 1)));
        _decayTime->setValue(rndr(0, 1));
        _sustainPunch->setValue(powf(frnd(0.8), 2));
        if (_attack->value(r) + _sustainTime->value(r) + _decayTime->value(r) < 0.2) {
            _sustainTime->setValue(_sustainTime->value(r) + 0.2 + frnd(0.3));
            _decayTime->setValue(_decayTime->value(r) + 0.2 + frnd(0.3));
        }
        _lpFiterResonance->setValue(rndr(-1, 1));
        _lpFilterCutoff->setValue(1 - powf(frnd(1), 3));
        _lpFilterCutoffSweep->setValue(powf(frnd(2) - 1, 3));
        if (_lpFilterCutoff->value(r) < 0.1 && _lpFilterCutoffSweep->value(r) < -0.05)
            _lpFilterCutoffSweep->setValue(-_lpFilterCutoffSweep->value(r));
        _hpFilterCutoff->setValue(powf(frnd(1), 5));
        _hpFilterCutoffSweep->setValue(powf(frnd(2) - 1, 5));
        _phaserOffset->setValue(powf(frnd(2) - 1, 3));
        _phaserSweep->setValue(powf(frnd(2) - 1, 3));
        _repeatSpeed->setValue(frnd(2) - 1);
        _changeSpeed->setValue(frnd(2) - 1);
        _changeAmount->setValue(frnd(2) - 1);
    }

#pragma mark ____________________________
#pragma mark Conversions

    // Conversion math was found here https://github.com/grumdrig/jsfxr/blob/master/sfxr.js
    // and inverted as necessary.

    const float OVERSAMPLING = 8.0f;

    float SfxrNode::envelopeTimeInSeconds(float p) {
        return p * p * 100000.0f / 44100.0f;
    }

    float SfxrNode::envelopeTimeInSfxrUnits(float p) {
        return sqrtf(p * 44100.0f / 100000.0f);
    }

    const float vibMagic = 44100 * 10 / 64.0f;
    float SfxrNode::vibratoInSfxrUnits(float hz) {
        float ret = hz * 100.0f;
        ret /= vibMagic;
        return sqrt(ret);
    }
    float SfxrNode::vibratoInHz(float sfxr) {
        float ret = vibMagic * sqr(sfxr) / 100;
        return ret;
    }

    float SfxrNode::frequencyInSfxrUnits(float p) {
        p /= OVERSAMPLING * 441.0f;
        p -= 0.001f;
        if (p < 0) p = 0;
        return sqrt(p);
    }
    float SfxrNode::frequencyInHz(float sfxr) {
        float p = sfxr * sfxr;
        p += 0.001f;
        p *= OVERSAMPLING * 441.0f;
        return p;
    }


    float SfxrNode::filterFreqInHz(float sfxr) {
        if (sfxr >= 1.0f)
            return 44100.0f;

        return OVERSAMPLING * 44100.0f * flurp(cube(sfxr) / 10);
    }
    float SfxrNode::filterFreqInSfxrUnits(float hz) {
        if (hz >= 44100.0f)
            return 1.0f;

        // if y = K x / (1 - x), then x = y / (y + K)
        float s3 = hz / (hz + (OVERSAMPLING * 44100.0f));
        return powf(s3, 1.0f/3.0f);
    }

#pragma mark ____________________________
#pragma mark Convenience

    void SfxrNode::setStartFrequencyInHz(float hz) {
        _startFrequency->setValue(frequencyInSfxrUnits(hz));
    }

    void SfxrNode::setVibratoSpeedInHz(float hz) {
        _vibratoSpeed->setValue(vibratoInSfxrUnits(hz));
    }


} // namespace lab
