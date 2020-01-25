
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#ifndef AudioDestinationMiniaudio_h
#define AudioDestinationMiniaudio_h

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioBus.h"

#include "internal/AudioDestination.h"
#include "internal/RingBuffer.h"

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

    float sampleRate() const override { return _sampleRate; }

    void render(int numberOfFrames, void * outputBuffer, void * inputBuffer);

    unsigned int channelCount() const { return _numChannels; }

private:

    void configure();

    RingBufferT<float>* _ring = nullptr;
    float * _scratch = nullptr;
    AudioIOCallback & _callback;
    AudioBus* _renderBus = nullptr;
    AudioBus* _inputBus = nullptr;
    uint32_t _numChannels = 0;
    uint32_t _numInputChannels = 0;
    float _sampleRate = 44100;
    ma_device _device;
    int _remainder = 0;
};

} // namespace lab

#endif // AudioDestinationMiniaudio_h

