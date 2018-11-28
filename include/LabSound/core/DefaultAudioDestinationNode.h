// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef DefaultAudioDestinationNode_h
#define DefaultAudioDestinationNode_h

#include "LabSound/core/AudioDestinationNode.h"

namespace lab {

class AudioContext;
struct AudioDestination;
    
class DefaultAudioDestinationNode final : public AudioDestinationNode 
{
    std::unique_ptr<AudioDestination> m_destination;

    void createDestination();
    
public:

    DefaultAudioDestinationNode(AudioContext* context, size_t channelCount, const float sampleRate);
    virtual ~DefaultAudioDestinationNode();
    
    virtual void initialize() override;
    virtual void uninitialize() override;
    virtual void startRendering() override;
    
    unsigned maxChannelCount() const;
    virtual void setChannelCount(ContextGraphLock &, size_t) override;
};

} // namespace lab

#endif
