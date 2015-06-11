// Copyright (c) 2003-2015 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef RecorderNode_h
#define RecorderNode_h

#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/AudioContext.h"
#include <vector>
#include <mutex>

namespace LabSound
{
    
    class RecorderNode : public WebCore::AudioBasicInspectorNode
    {
        
    public:
        
        RecorderNode(float sampleRate);
        virtual ~RecorderNode();
        
        // AudioNode
        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;
        
        void startRecording() { m_recording = true; }
        void stopRecording() { m_recording = false; }
        
        void mixToMono(bool m) { m_mixToMono = m; }

        // replaces result with the currently recorded data.
        // saved data is cleared.
        void getData(std::vector<float>& result);
        
        void writeRecordingToWav(int channels, const std::string & filenameWithWavExtension);
        
    private:
        
        virtual double tailTime() const override { return 0; }
        virtual double latencyTime() const override { return 0; }

        bool m_mixToMono;
        bool m_recording;

        std::vector<float> m_data;  // interleaved
        mutable std::recursive_mutex m_mutex;

    };
    
} // end namespace LabSound

#endif
