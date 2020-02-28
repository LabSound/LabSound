// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef audio_hardware_io_node_hpp
#define audio_hardware_io_node_hpp

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNode.h"

namespace lab
{

class AudioBus;
class AudioContext;
class AudioHardwareInput;
struct AudioSourceProvider;

class AudioHardwareDeviceNode : public AudioNode, public AudioDeviceRenderCallback
{
protected:
    // AudioNode interface
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    AudioContext * m_context;

    // Platform specific implementation
    std::unique_ptr<AudioDevice> m_platformAudioDevice;
    AudioHardwareInput * m_audioHardwareInput{nullptr};
    SamplingInfo last_info;

    const AudioStreamConfig outConfig;
    const AudioStreamConfig inConfig;

public:
    AudioHardwareDeviceNode(AudioContext * context, const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig);
    virtual ~AudioHardwareDeviceNode();

    // AudioNode interface
    virtual void process(ContextRenderLock &, size_t) override {}  // AudioHardwareDeviceNode is pulled by hardware so this is never called
    virtual void reset(ContextRenderLock &) override;
    virtual void initialize() override;
    virtual void uninitialize() override;

    // AudioDeviceRenderCallback interface
    virtual void render(AudioBus * src, AudioBus * dst, size_t frames, const SamplingInfo & info) override;
    virtual void start() override;
    virtual void stop() override;
    virtual const SamplingInfo getSamplingInfo() const override;
    virtual const AudioStreamConfig getOutputConfig() const override;
    virtual const AudioStreamConfig getInputConfig() const override;

    AudioSourceProvider * AudioHardwareInputProvider();
};

}  // namespace lab

#endif  // end audio_hardware_io_node_hpp
