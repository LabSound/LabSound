// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef OfflineAudioDestinationNode_h
#define OfflineAudioDestinationNode_h

#include "LabSound/core/AudioDestinationNode.h"

namespace lab {

class AudioBus;
class AudioContext;
    
class OfflineAudioDestinationNode : public AudioDestinationNode
{
    
public:
    
    OfflineAudioDestinationNode(AudioContext * context, AudioBuffer * renderTarget);
    virtual ~OfflineAudioDestinationNode();
    
    virtual void initialize();
    virtual void uninitialize();
    virtual float sampleRate() const { return m_renderTarget->sampleRate(); }

    void startRendering();
    
private:
    
    // This AudioNode renders into this AudioBuffer.
    AudioBuffer * m_renderTarget;
    
    // Temporary AudioBus for each render quantum.
    std::unique_ptr<AudioBus> m_renderBus;
    
    // Rendering thread.
    std::thread m_renderThread;
    
    bool m_startedRendering{ false };
    void offlineRender();
    
    AudioContext * ctx;
    
};

} // namespace lab

#endif
