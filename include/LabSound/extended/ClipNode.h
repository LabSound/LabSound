
// Copyright (c) 2003-2015 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

// ClipNode clips a signal, using either thresholding or tanh

#ifndef _LABSOUND_CLIP_NODE_H
#define _LABSOUND_CLIP_NODE_H

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioParam.h"

namespace LabSound
{

    class ClipNode : public WebCore::AudioBasicProcessorNode
    {
        class ClipNodeInternal;
        ClipNodeInternal * internalNode;
        
    public:
        enum Mode { CLIP, TANH };

        ClipNode(float sampleRate);
        virtual ~ClipNode();

        void setMode(Mode m);

        // in CLIP mode, a is the min value, and b is the max value.
        // in TANH mode, a is the overall gain, and b is the input gain.
        // The higher the input gain the more severe the distortion.
        std::shared_ptr<WebCore::AudioParam> aVal();
		std::shared_ptr<WebCore::AudioParam> bVal();
    };
    
}

#endif
