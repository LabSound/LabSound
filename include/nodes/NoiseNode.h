
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "AudioScheduledSourceNode.h"
#include "AudioNode.h"
#include "AudioParam.h"
#include "ExceptionCodes.h"

namespace LabSound {


    class NoiseNode : public WebCore::AudioScheduledSourceNode {
    public:
        // The noise type.
        enum {
            WHITE = 0,
            PINK = 1,
            BROWN = 2
        };

        NoiseNode(float sampleRate);
        virtual ~NoiseNode();

        // AudioNode
        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(std::shared_ptr<AudioContext>) override;

        unsigned short type() const { return m_type; }
        void setType(unsigned short, ExceptionCode&);

    private:

        virtual bool propagatesSilence(double now) const OVERRIDE;

        // One of the noise types defined in the enum.
        unsigned short m_type;

        uint32_t whiteSeed;
        float lastBrown;
        float pink0, pink1, pink2, pink3, pink4, pink5, pink6;
    };


}
