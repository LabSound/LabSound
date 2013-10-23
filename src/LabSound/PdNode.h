
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

// PdNode wraps an instance of pure-data as a signal processing node

#pragma once

#include "AudioBasicProcessorNode.h"

namespace pd {
    class PdBase;
    class PdMidiReceiver;
    class PdReceiver;
}

namespace LabSound {

class PdNode : public WebCore::AudioBasicProcessorNode
{
public:
    static WTF::PassRefPtr<PdNode> create(WebCore::AudioContext* context, float sampleRate)
    {
        return adoptRef(new PdNode(context, sampleRate));
    }
    
    pd::PdBase& pd() const;
    
    virtual bool propagatesSilence() const OVERRIDE;
    
private:
    PdNode(WebCore::AudioContext*, float sampleRate);
    virtual ~PdNode();
    
    class PdNodeInternal;
    PdNodeInternal* data;
};

}
