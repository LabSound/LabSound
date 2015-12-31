// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef DefaultAudioDestinationNode_h
#define DefaultAudioDestinationNode_h

#include "LabSound/core/AudioDestinationNode.h"

namespace lab {

class AudioContext;
struct AudioDestination;
    
class DefaultAudioDestinationNode : public AudioDestinationNode 
{
    std::unique_ptr<AudioDestination> m_destination;

    void createDestination();
    
public:

    DefaultAudioDestinationNode(std::shared_ptr<AudioContext>);
    virtual ~DefaultAudioDestinationNode();
    
    virtual void initialize() override;
    virtual void uninitialize() override;
    virtual void startRendering() override;
    
    unsigned maxChannelCount() const;
    virtual void setChannelCount(ContextGraphLock&, unsigned long) override;
};

} // namespace lab

#endif
