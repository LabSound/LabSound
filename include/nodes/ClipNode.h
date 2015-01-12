
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

        ClipNode(float sampleRate);
        virtual ~ClipNode();

        void setMode(Mode);

        // in CLIP mode, a is the min value, and b is the max value.
        // in TANH mode, a is the overall gain, and b is the input gain.
        //   The higher the input gain the more severe the distortion.
        std::shared_ptr<AudioParam> aVal();
		std::shared_ptr<AudioParam> bVal();

    private:
        class ClipNodeInternal;
        ClipNodeInternal* data;
    };
    
}
