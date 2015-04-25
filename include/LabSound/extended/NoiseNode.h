// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"

#include "LabSound/extended/ExceptionCodes.h"

namespace LabSound 
{

    class NoiseNode : public WebCore::AudioScheduledSourceNode 
	{

    public:

        enum NoiseType
		{
            WHITE,
            PINK,
            BROWN
        };

        NoiseNode(float sampleRate);
        virtual ~NoiseNode();

        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;

        unsigned short type() const { return m_type; }

        void setType(NoiseType newType, ExceptionCode&);

    private:

        virtual bool propagatesSilence(double now) const override;

        NoiseType m_type = WHITE;

        uint32_t whiteSeed = 1489853723;

        float lastBrown = 0;

        float pink0 = 0; 
		float pink1 = 0;
		float pink2 = 0;
		float pink3 = 0;
		float pink4 = 0;
		float pink5 = 0;
		float pink6 = 0;

    };

}
