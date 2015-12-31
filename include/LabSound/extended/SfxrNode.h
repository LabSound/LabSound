// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef SFXR_NODE_H
#define SFXR_NODE_H

#include "LabSound/core/AudioScheduledSourceNode.h"

namespace lab {

    class SfxrNode : public lab::AudioScheduledSourceNode {
    public:

        SfxrNode(float sampleRate);
        virtual ~SfxrNode();

        // AudioNode
        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;

        // SfxrNode - values in sfxr units
        std::shared_ptr<AudioParam> waveType() { return _waveType; }

        std::shared_ptr<AudioParam> attackTime() { return _attack; }
        std::shared_ptr<AudioParam> sustainTime() { return _sustainTime; }
        std::shared_ptr<AudioParam> sustainPunch() { return _sustainPunch; }
        std::shared_ptr<AudioParam> decayTime() { return _decayTime; }

        std::shared_ptr<AudioParam> startFrequency() { return _startFrequency; }
        std::shared_ptr<AudioParam> minFrequency() { return _minFrequency; }
        std::shared_ptr<AudioParam> slide() { return _slide; }
        std::shared_ptr<AudioParam> deltaSlide() { return _deltaSlide; }

        std::shared_ptr<AudioParam> vibratoDepth() { return _vibratoDepth; }
        std::shared_ptr<AudioParam> vibratoSpeed() { return _vibratoSpeed; }

        std::shared_ptr<AudioParam> changeAmount() { return _changeAmount; }
        std::shared_ptr<AudioParam> changeSpeed() { return _changeSpeed; }

        std::shared_ptr<AudioParam> squareDuty() { return _squareDuty; }
        std::shared_ptr<AudioParam> dutySweep() { return _dutySweep; }

        std::shared_ptr<AudioParam> repeatSpeed() { return _repeatSpeed; }
        std::shared_ptr<AudioParam> phaserOffset() { return _phaserOffset; }
        std::shared_ptr<AudioParam> phaserSweep() { return _phaserSweep; }

        std::shared_ptr<AudioParam> lpFilterCutoff() { return _lpFilterCutoff; }
        std::shared_ptr<AudioParam> lpFilterCutoffSweep() { return _lpFilterCutoffSweep; }
        std::shared_ptr<AudioParam> lpFiterResonance() { return _lpFiterResonance; }
        std::shared_ptr<AudioParam> hpFilterCutoff() { return _hpFilterCutoff; }
        std::shared_ptr<AudioParam> hpFilterCutoffSweep() { return _hpFilterCutoffSweep; }

        void setStartFrequencyInHz(float);
        void setVibratoSpeedInHz(float);

        // sfxr uses a lot of weird parameters. These are utility functions to help with that.
        float envelopeTimeInSeconds(float sfxrEnvTime);
        float envelopeTimeInSfxrUnits(float t);
        float frequencyInSfxrUnits(float hz);
        float frequencyInHz(float sfxr);
        float vibratoInSfxrUnits(float hz);
        float vibratoInHz(float sfxr);
        float filterFreqInHz(float sfxr);
        float filterFreqInSfxrUnits(float hz);

        enum WaveType { SQUARE = 0, SAWTOOTH, SINE, NOISE };

        void noteOn();

        // some presets
        void setDefaultBeep();
        void coin();
        void laser(ContextRenderLock&);
        void explosion();
        void powerUp();
        void hit(ContextRenderLock&);
        void jump();
        void select(ContextRenderLock&);

        // mutate the current sound
        void mutate(ContextRenderLock&);
        void randomize(ContextRenderLock&);

    private:
        virtual bool propagatesSilence(double now) const override;

        std::shared_ptr<AudioParam> _waveType;
        std::shared_ptr<AudioParam> _attack;
        std::shared_ptr<AudioParam> _sustainTime;
        std::shared_ptr<AudioParam> _sustainPunch;
        std::shared_ptr<AudioParam> _decayTime;
        std::shared_ptr<AudioParam> _startFrequency;
        std::shared_ptr<AudioParam> _minFrequency;
        std::shared_ptr<AudioParam> _slide;
        std::shared_ptr<AudioParam> _deltaSlide;
        std::shared_ptr<AudioParam> _vibratoDepth;
        std::shared_ptr<AudioParam> _vibratoSpeed;
        std::shared_ptr<AudioParam> _changeAmount;
        std::shared_ptr<AudioParam> _changeSpeed;
        std::shared_ptr<AudioParam> _squareDuty;
        std::shared_ptr<AudioParam> _dutySweep;
        std::shared_ptr<AudioParam> _repeatSpeed;
        std::shared_ptr<AudioParam> _phaserOffset;
        std::shared_ptr<AudioParam> _phaserSweep;
        std::shared_ptr<AudioParam> _lpFilterCutoff;
        std::shared_ptr<AudioParam> _lpFilterCutoffSweep;
        std::shared_ptr<AudioParam> _lpFiterResonance;
        std::shared_ptr<AudioParam> _hpFilterCutoff;
        std::shared_ptr<AudioParam> _hpFilterCutoffSweep;

        class Sfxr;
        Sfxr *sfxr;
    };
    
}

#endif
