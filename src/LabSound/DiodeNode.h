//
//  DiodeNode.h
//  LabSound
//
//  Created by Nick Porcino on 2012 12/19.
//
//

#ifndef LabSound_DiodeNode_h
#define LabSound_DiodeNode_h

#include "WaveShaperNode.h"

namespace LabSound {

    class DiodeNode
    {
    public:
        DiodeNode(WebCore::AudioContext*);
        
        void setDistortion(float distortion);
        WTF::PassRefPtr<WebCore::WaveShaperNode> node() const { return waveShaper; }

    private:
        void setCurve();

        WTF::RefPtr<WebCore::WaveShaperNode> waveShaper;
        
        // parameters controlling the shape of the curve
        float vb;
        float vl;
        float h;
    };
    
}

#endif
