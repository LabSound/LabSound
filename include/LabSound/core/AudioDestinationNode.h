// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioDestinationNode_h
#define AudioDestinationNode_h

#include "LabSound/core/AudioBuffer.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioIOCallback.h"


namespace lab {

class AudioBus;
class AudioContext;
class AudioSourceProvider;
class LocalAudioInputProvider;

class AudioDestinationNode : public AudioNode, public AudioIOCallback {

    class LocalAudioInputProvider;
    LocalAudioInputProvider * m_localAudioInputProvider;

public:

    AudioDestinationNode(std::shared_ptr<AudioContext>, float sampleRate);

    virtual ~AudioDestinationNode();
    
    // AudioNode   
    virtual void process(ContextRenderLock&, size_t) override { } // we're pulled by hardware so this is never called
    virtual void reset(ContextRenderLock&) override { m_currentSampleFrame = 0; };
    
    // The audio hardware calls render() to get the next render quantum of audio into destinationBus.
    // It will optionally give us local/live audio input in sourceBus (if it's not 0).
    virtual void render(AudioBus* sourceBus, AudioBus* destinationBus, size_t numberOfFrames) override;

    size_t currentSampleFrame() const { return m_currentSampleFrame; }
    double currentTime() const { return currentSampleFrame() / static_cast<double>(sampleRate()); }

    virtual unsigned numberOfChannels() const { return 2; } // FIXME: update when multi-channel (more than stereo) is supported

    virtual void startRendering() = 0;

    AudioSourceProvider * localAudioInputProvider();
    
protected:

    virtual double tailTime() const override { return 0; }
    virtual double latencyTime() const override { return 0; }

    // Counts the number of sample-frames processed by the destination.
    size_t m_currentSampleFrame;

    std::shared_ptr<AudioContext> m_context;

};

} // namespace lab

#endif // AudioDestinationNode_h
