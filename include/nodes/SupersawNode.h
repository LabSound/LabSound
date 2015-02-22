//
//  SupersawNode.h
//
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef __LabSound__SupersawNode__
#define __LabSound__SupersawNode__

#include "AudioContext.h"
#include "AudioNode.h"
#include "AudioParam.h"

namespace LabSound {
    using namespace WebCore;

    class SupersawNode : public AudioNode {
    public:
        SupersawNode(ContextGraphLock& g, ContextRenderLock& r, float sampleRate);
        virtual ~SupersawNode() {}
        
		std::shared_ptr<AudioParam> attack()  const;
		std::shared_ptr<AudioParam> decay()   const;
		std::shared_ptr<AudioParam> sustain() const;
		std::shared_ptr<AudioParam> release() const;

        std::shared_ptr<AudioParam> sawCount()  const;
        std::shared_ptr<AudioParam> frequency() const;
        std::shared_ptr<AudioParam> detune()    const;

		void noteOn(double when);
		void noteOff(ContextRenderLock&, double when);

        void update(ContextGraphLock& g, ContextRenderLock& r); // call if sawCount is changed. CBB: update automatically

    private:

        // Satisfy the AudioNode interface
        virtual void process(ContextGraphLock& g, ContextRenderLock&, size_t) override;
        virtual void reset(std::shared_ptr<AudioContext>) override { /*m_currentSampleFrame = 0;*/ }
        virtual double tailTime() const OVERRIDE { return 0; }
        virtual double latencyTime() const OVERRIDE { return 0; }
        virtual bool propagatesSilence(double now) const OVERRIDE;
        // ^

        class Data;
        Data* _data;
    };
}

#endif
