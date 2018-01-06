// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef OfflineAudioDestinationNode_h
#define OfflineAudioDestinationNode_h

#include "LabSound/core/AudioDestinationNode.h"

namespace lab {

class AudioBus;
class AudioContext;

class OfflineAudioDestinationNode final : public AudioDestinationNode
{
    
public:
    
    OfflineAudioDestinationNode(AudioContext * context, const float lengthSeconds, const uint32_t numChannels, const float sampleRate);
    virtual ~OfflineAudioDestinationNode();
    
    virtual void initialize();
    virtual void uninitialize();
    virtual float sampleRate() const { return m_sampleRate; }

    void startRendering();
    
private:
  
    std::unique_ptr<AudioBus> m_renderBus;
    std::thread m_renderThread;
    void offlineRender();
    bool m_startedRendering{ false };
    float m_sampleRate;
    uint32_t m_numChannels;
    float m_lengthSeconds;
    AudioContext * ctx;
};

} // namespace lab

#endif
