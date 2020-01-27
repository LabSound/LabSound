// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef labsound_null_device_node_h
#define labsound_null_device_node_h

#include "LabSound/core/AudioHardwareDeviceNode.h"
#include "LabSound/core/AudioIOCallback.h"

namespace lab
{
    class AudioBus;
    class AudioContext;

    class NullDeviceNode final : public AudioNode, public AudioDeviceRenderCallback 
    {
        std::unique_ptr<AudioBus> m_renderBus;
        std::thread m_renderThread;

        void offlineRender();
        bool m_startedRendering{false};
        uint32_t m_numChannels;
        float m_lengthSeconds;

        AudioContext * m_context;

    public:

        NullDeviceNode(AudioContext * context, const float sampleRate, const float lengthSeconds, const uint32_t numChannels);
        virtual ~NullDeviceNode();

        // AudioNode Interface
        virtual void initialize() override;
        virtual void uninitialize() override;

        // AudioDeviceRenderCallback interface
        virtual void render(AudioBus * sourceBus, AudioBus * destinationBus, size_t numberOfFrames) override;
        virtual void start() override;
        virtual void stop() override;
        virtual uint64_t currentSampleFrame() const override;
        virtual double currentTime() const override;
        virtual double currentSampleTime() const override;
    };

}  // namespace lab

#endif // end labsound_null_device_node_h
