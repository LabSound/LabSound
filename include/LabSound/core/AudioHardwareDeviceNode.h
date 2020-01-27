// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef audio_hardware_io_node_hpp
#define audio_hardware_io_node_hpp

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioDevice.h"


namespace lab {

class AudioBus;
class AudioContext;
struct AudioSourceProvider;
class LocalAudioInputProvider;

class AudioHardwareDeviceNode : public AudioNode, public AudioDeviceRenderCallback 
{
protected:

    class LocalAudioInputProvider;
    LocalAudioInputProvider * m_localAudioInputProvider;

    // AudioNode interface 
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    float m_sampleRate;
    AudioContext * m_context;

    // Platform specific implementation
    std::unique_ptr<AudioDevice> m_destination;

public:

    AudioHardwareDeviceNode(AudioContext * context, size_t channelCount, float sampleRate);
    virtual ~AudioHardwareDeviceNode();
    
    // AudioNode interface  
    virtual void process(ContextRenderLock &, size_t) override { } // AudioHardwareDeviceNode is pulled by hardware so this is never called
    virtual void reset(ContextRenderLock &) override;
    virtual void initialize() override;
    virtual void uninitialize() override;

    // AudioDeviceRenderCallback interface
    virtual void render(AudioBus * sourceBus, AudioBus * destinationBus, size_t numberOfFrames) override;
    virtual void start() override;
    virtual void stop() override;
    virtual uint64_t currentSampleFrame() const override;
    virtual double currentTime() const override;
    virtual double currentSampleTime() const override;

    size_t numberOfChannels() const { return m_channelCount; }
    float sampleRate() const { return m_sampleRate; }

    AudioSourceProvider * localAudioInputProvider();

    // No
    virtual void setChannelCount(ContextGraphLock &, size_t) override;
};

} // namespace lab

#endif // end audio_hardware_io_node_hpp
