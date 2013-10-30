
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

// ClipNode clips a signal, using either thresholding or tanh

#pragma once

#include "AudioBasicProcessorNode.h"
#include "AudioParam.h"

namespace LabSound {

    class ClipNode : public WebCore::AudioBasicProcessorNode
    {
    public:
        enum Mode { CLIP, TANH };

        static WTF::PassRefPtr<ClipNode> create(WebCore::AudioContext* context, float sampleRate)
        {
            return adoptRef(new ClipNode(context, sampleRate));
        }

        void setMode(Mode);

        WebCore::AudioParam* minVal();
		WebCore::AudioParam* maxVal();

    private:
        ClipNode(WebCore::AudioContext*, float sampleRate);
        virtual ~ClipNode();
        
        class ClipNodeInternal;
        ClipNodeInternal* data;
    };
    
}
