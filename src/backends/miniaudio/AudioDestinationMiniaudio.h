// License: BSD 3 Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#ifndef AudioDestinationMiniaudio_h
#define AudioDestinationMiniaudio_h

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioBus.h"

#include "internal/AudioDestination.h"

#include "miniaudio.h"
#include <iostream>
#include <memory>
#include <cstdlib>

namespace lab {

class AudioDestinationMiniaudio : public AudioDestination
{
public:

#ifdef _MSC_VER
    // satisfy ma_device alignment requirements for msvc.
    void * operator new(size_t i) { return _mm_malloc(i, 64); }
    void operator delete(void * p) { _mm_free(p); }
#endif

    AudioDestinationMiniaudio(AudioIOCallback &,
                              uint32_t numChannels, uint32_t numInputChannels, float sampleRate);
    virtual ~AudioDestinationMiniaudio();

    virtual void start() override;
    virtual void stop() override;

    float sampleRate() const override { return m_sampleRate; }

    void render(int numberOfFrames, void * outputBuffer, void * inputBuffer);

    unsigned int channelCount() const { return m_numChannels; }

private:

    void configure();

    AudioIOCallback & m_callback;
    AudioBus* m_renderBus = nullptr;
    AudioBus* m_inputBus = nullptr;
    uint32_t m_numChannels = 0;
    uint32_t m_numInputChannels = 0;
    float m_sampleRate = 44000;
    ma_device device;
};

} // namespace lab

#endif // AudioDestinationMiniaudio_h

