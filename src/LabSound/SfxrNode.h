
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include <wtf/Forward.h>
#include <AudioScheduledSourceNode.h>

namespace LabSound {

    using namespace WebCore;

    class SfxrNode : public WebCore::AudioScheduledSourceNode {
    public:

        static WTF::PassRefPtr<SfxrNode> create(WebCore::AudioContext*, float sampleRate);

        virtual ~SfxrNode();

        // AudioNode
        virtual void process(size_t framesToProcess);
        virtual void reset();

        // SfxrNode
        AudioParam* waveType() { return _waveType.get(); }

		AudioParam* attackTime() { return _attack.get(); }
		AudioParam* sustainTime() { return _sustainTime.get(); }
		AudioParam* sustainPunch() { return _sustainPunch.get(); }
		AudioParam* decayTime() { return _decayTime.get(); }

		AudioParam* startFrequency() { return _startFrequency.get(); }
		AudioParam* minFrequency() { return _minFrequency.get(); }
		AudioParam* slide() { return _slide.get(); }
		AudioParam* deltaSlide() { return _deltaSlide.get(); }

		AudioParam* vibratoDepth() { return _vibratoDepth.get(); }
		AudioParam* vibratoSpeed() { return _vibratoSpeed.get(); }
        AudioParam* vibratoDelay() { return _vibratoDelay.get(); }

		AudioParam* changeAmount() { return _changeAmount.get(); }
		AudioParam* changeSpeed() { return _changeSpeed.get(); }

		AudioParam* squareDuty() { return _squareDuty.get(); }
		AudioParam* dutySweep() { return _dutySweep.get(); }

		AudioParam* repeatSpeed() { return _repeatSpeed.get(); }
		AudioParam* phaserOffset() { return _phaserOffset.get(); }
		AudioParam* phaserSweep() { return _phaserSweep.get(); }

		AudioParam* lpFilterCutoff() { return _lpFilterCutoff.get(); }
		AudioParam* lpFilterCutoffSweep() { return _lpFilterCutoffSweep.get(); }
		AudioParam* lpFiterResonance() { return _lpFiterResonance.get(); }
		AudioParam* hpFilterCutoff() { return _hpFilterCutoff.get(); }
		AudioParam* hpFilterCutoffSweep() { return _hpFilterCutoffSweep.get(); }

        enum WaveType { SQUARE = 0, SAWTOOTH, SINE, NOISE };

        void noteOn();

        // some presets
        void setDefaultBeep();
        void pickupCoin();
        void laserShot();
        void explosion();
        void powerUp();
        void hit();
        void jump();
        void select();

        // mutate the current sound
        void mutate();
        void randomize();

    private:
        SfxrNode(WebCore::AudioContext*, float sampleRate);
        virtual bool propagatesSilence() const OVERRIDE;

        RefPtr<AudioParam> _waveType;
		RefPtr<AudioParam> _attack;
		RefPtr<AudioParam> _sustainTime;
		RefPtr<AudioParam> _sustainPunch;
		RefPtr<AudioParam> _decayTime;
		RefPtr<AudioParam> _startFrequency;
		RefPtr<AudioParam> _minFrequency;
		RefPtr<AudioParam> _slide;
		RefPtr<AudioParam> _deltaSlide;
		RefPtr<AudioParam> _vibratoDepth;
		RefPtr<AudioParam> _vibratoSpeed;
        RefPtr<AudioParam> _vibratoDelay;
		RefPtr<AudioParam> _changeAmount;
		RefPtr<AudioParam> _changeSpeed;
		RefPtr<AudioParam> _squareDuty;
		RefPtr<AudioParam> _dutySweep;
		RefPtr<AudioParam> _repeatSpeed;
		RefPtr<AudioParam> _phaserOffset;
		RefPtr<AudioParam> _phaserSweep;
		RefPtr<AudioParam> _lpFilterCutoff;
		RefPtr<AudioParam> _lpFilterCutoffSweep;
		RefPtr<AudioParam> _lpFiterResonance;
		RefPtr<AudioParam> _hpFilterCutoff;
		RefPtr<AudioParam> _hpFilterCutoffSweep;

        class Sfxr;
        Sfxr *sfxr;
    };
    
    
}
