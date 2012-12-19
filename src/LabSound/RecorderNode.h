//
//  RecorderNode.h
//  LabSound
//
//  Created by Nick Porcino on 2012 12/17.
//
//

#ifndef LabSound_RecorderNode_h
#define LabSound_RecorderNode_h

#include "AudioBasicInspectorNode.h"
#include <wtf/Forward.h>
#include <vector>

namespace LabSound {
    
    class RecorderNode : public WebCore::AudioBasicInspectorNode {
    public:
        static WTF::PassRefPtr<RecorderNode> create(WebCore::AudioContext* context, float sampleRate)
        {
            return adoptRef(new RecorderNode(context, sampleRate));
        }
        
        virtual ~RecorderNode();
        
        // AudioNode
        virtual void process(size_t framesToProcess);
        virtual void reset();
        
        void startRecording() { m_recording = true; }
        void stopRecording() { m_recording = false; }
        
        void save(char const*const path);
        
    private:
        virtual double tailTime() const OVERRIDE { return 0; }
        virtual double latencyTime() const OVERRIDE { return 0; }
        
        RecorderNode(WebCore::AudioContext*, float sampleRate);
        
        bool m_recording;
        std::vector<float> m_data;  // interleaved
    };
    
} // LabSound

#endif
