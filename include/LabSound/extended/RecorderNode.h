// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef RECORDER_NODE_H
#define RECORDER_NODE_H

#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/AudioContext.h"
#include <vector>
#include <mutex>

namespace lab
{
    
    class RecorderNode : public AudioBasicInspectorNode
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
    
} // end namespace lab

#endif
