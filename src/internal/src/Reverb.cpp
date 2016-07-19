// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/Reverb.h"
#include "internal/AudioBus.h"
#include "internal/AudioFileReader.h"
#include "internal/ReverbConvolver.h"
#include "internal/VectorMath.h"
#include "internal/ConfigMacros.h"

#include <math.h>
#include <WTF/MathExtras.h>

#if OS(DARWIN)
using namespace std;
#endif

namespace lab {

using namespace VectorMath;

// Empirical gain calibration tested across many impulse responses to ensure perceived volume is same as dry (unprocessed) signal
const float GainCalibration = -58;
const float GainCalibrationSampleRate = 44100;

// A minimum power value to when normalizing a silent (or very quiet) impulse response
const float MinPower = 0.000125f;
    
static float calculateNormalizationScale(AudioBus* response)
{
    // Normalize by RMS power
    size_t numberOfChannels = response->numberOfChannels();
    size_t length = response->length();

    float power = 0;

    for (size_t i = 0; i < numberOfChannels; ++i) {
        float channelPower = 0;
        vsvesq(response->channel(i)->data(), 1, &channelPower, length);
        power += channelPower;
    }

    power = sqrt(power / (numberOfChannels * length));

    // Protect against accidental overload
    if (std::isinf(power) || std::isnan(power) || power < MinPower)
        power = MinPower;

    float scale = 1 / power;

    scale *= powf(10, GainCalibration * 0.05f); // calibrate to make perceived volume same as unprocessed

    // Scale depends on sample-rate.
    if (response->sampleRate())
        scale *= GainCalibrationSampleRate / response->sampleRate();

    // True-stereo compensation
    if (response->numberOfChannels() == 4)
        scale *= 0.5f;

    return scale;
}

Reverb::Reverb(AudioBus* impulseResponse, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads, bool normalize)
{
    float scale = 1;

    if (normalize) {
        scale = calculateNormalizationScale(impulseResponse);

        if (scale)
            impulseResponse->scale(scale);
    }

    initialize(impulseResponse, renderSliceSize, maxFFTSize, numberOfChannels, useBackgroundThreads);

    // Undo scaling since this shouldn't be a destructive operation on impulseResponse.
    // FIXME: What about roundoff? Perhaps consider making a temporary scaled copy
    // instead of scaling and unscaling in place.
    if (normalize && scale)
        impulseResponse->scale(1 / scale);
}

void Reverb::initialize(AudioBus* impulseResponseBuffer, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads)
{
    m_impulseResponseLength = impulseResponseBuffer->length();

    // The reverb can handle a mono impulse response and still do stereo processing
    size_t numResponseChannels = impulseResponseBuffer->numberOfChannels();
    m_convolvers.reserve(numberOfChannels);

    int convolverRenderPhase = 0;
    for (size_t i = 0; i < numResponseChannels; ++i) {
        AudioChannel* channel = impulseResponseBuffer->channel(i);

        m_convolvers.push_back(
           std::unique_ptr<ReverbConvolver>(
               new ReverbConvolver(channel, renderSliceSize, maxFFTSize,
                                  convolverRenderPhase, useBackgroundThreads)));

        convolverRenderPhase += renderSliceSize;
    }

    // For "True" stereo processing we allocate a temporary buffer to avoid repeatedly allocating it in the process() method.
    // It can be bad to allocate memory in a real-time thread.
    if (numResponseChannels == 4)
        m_tempBuffer = std::unique_ptr<AudioBus>(new AudioBus(2, MaxFrameSize));
}

void Reverb::process(ContextRenderLock& r, const AudioBus* sourceBus, AudioBus* destinationBus, size_t framesToProcess)
{
    // Do a fairly comprehensive sanity check.
    // If these conditions are satisfied, all of the source and destination pointers will be valid for the various matrixing cases.
    bool isSafeToProcess = sourceBus && destinationBus && sourceBus->numberOfChannels() > 0 && destinationBus->numberOfChannels() > 0
        && framesToProcess <= MaxFrameSize && framesToProcess <= sourceBus->length() && framesToProcess <= destinationBus->length(); 
    
    ASSERT(isSafeToProcess);
    if (!isSafeToProcess)
        return;

    // For now only handle mono or stereo output
    if (destinationBus->numberOfChannels() > 2) {
        destinationBus->zero();
        return;
    }

    AudioChannel* destinationChannelL = destinationBus->channelByType(Channel::Left);
    const AudioChannel* sourceChannelL = sourceBus->channelByType(Channel::Left);

    // Handle input -> output matrixing...
    size_t numInputChannels = sourceBus->numberOfChannels();
    size_t numOutputChannels = destinationBus->numberOfChannels();
    size_t numReverbChannels = m_convolvers.size();

    if (numInputChannels == 2 && numReverbChannels == 2 && numOutputChannels == 2) {
        // 2 -> 2 -> 2
        const AudioChannel* sourceChannelR = sourceBus->channelByType(Channel::Right);
        AudioChannel* destinationChannelR = destinationBus->channelByType(Channel::Right);
        m_convolvers[0]->process(r, sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[1]->process(r, sourceChannelR, destinationChannelR, framesToProcess);
    } else if (numInputChannels == 2 && numReverbChannels == 1 && numOutputChannels == 2) {
        // LabSound added this case, should submit it back to WebKit after it's known to work correctly
        // because the initialize method says that a mono-IR is expected to work with a stero in/out setup
        // 2 -> 1 -> 2
        const AudioChannel* sourceChannelR = sourceBus->channelByType(Channel::Right);
        AudioChannel* destinationChannelR = destinationBus->channelByType(Channel::Right);
        m_convolvers[0]->process(r, sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[0]->process(r, sourceChannelR, destinationChannelR, framesToProcess);
    } else  if (numInputChannels == 1 && numOutputChannels == 2 && numReverbChannels == 2) {
        // 1 -> 2 -> 2
        for (int i = 0; i < 2; ++i) {
            AudioChannel* destinationChannel = destinationBus->channel(i);
            m_convolvers[i]->process(r, sourceChannelL, destinationChannel, framesToProcess);
        }
    } else if (numInputChannels == 1 && numReverbChannels == 1 && numOutputChannels == 2) {
        // 1 -> 1 -> 2
        m_convolvers[0]->process(r, sourceChannelL, destinationChannelL, framesToProcess);

        // simply copy L -> R
        AudioChannel* destinationChannelR = destinationBus->channelByType(Channel::Right);
        bool isCopySafe = destinationChannelL->data() && destinationChannelR->data() && destinationChannelL->length() >= framesToProcess && destinationChannelR->length() >= framesToProcess;
        ASSERT(isCopySafe);
        if (!isCopySafe)
            return;
        memcpy(destinationChannelR->mutableData(), destinationChannelL->data(), sizeof(float) * framesToProcess);
    } else if (numInputChannels == 1 && numReverbChannels == 1 && numOutputChannels == 1) {
        // 1 -> 1 -> 1
        m_convolvers[0]->process(r, sourceChannelL, destinationChannelL, framesToProcess);
    } else if (numInputChannels == 2 && numReverbChannels == 4 && numOutputChannels == 2) {
        // 2 -> 4 -> 2 ("True" stereo)
        const AudioChannel* sourceChannelR = sourceBus->channelByType(Channel::Right);
        AudioChannel* destinationChannelR = destinationBus->channelByType(Channel::Right);

        AudioChannel* tempChannelL = m_tempBuffer->channelByType(Channel::Left);
        AudioChannel* tempChannelR = m_tempBuffer->channelByType(Channel::Right);

        // Process left virtual source
        m_convolvers[0]->process(r, sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[1]->process(r, sourceChannelL, destinationChannelR, framesToProcess);

        // Process right virtual source
        m_convolvers[2]->process(r, sourceChannelR, tempChannelL, framesToProcess);
        m_convolvers[3]->process(r, sourceChannelR, tempChannelR, framesToProcess);

        destinationBus->sumFrom(*m_tempBuffer);
    } else if (numInputChannels == 1 && numReverbChannels == 4 && numOutputChannels == 2) {
        // 1 -> 4 -> 2 (Processing mono with "True" stereo impulse response)
        // This is an inefficient use of a four-channel impulse response, but we should handle the case.
        AudioChannel* destinationChannelR = destinationBus->channelByType(Channel::Right);

        AudioChannel* tempChannelL = m_tempBuffer->channelByType(Channel::Left);
        AudioChannel* tempChannelR = m_tempBuffer->channelByType(Channel::Right);

        // Process left virtual source
        m_convolvers[0]->process(r, sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[1]->process(r, sourceChannelL, destinationChannelR, framesToProcess);

        // Process right virtual source
        m_convolvers[2]->process(r, sourceChannelL, tempChannelL, framesToProcess);
        m_convolvers[3]->process(r, sourceChannelL, tempChannelR, framesToProcess);

        destinationBus->sumFrom(*m_tempBuffer);
    } else {
        // Handle gracefully any unexpected / unsupported matrixing
        // FIXME: add code for 5.1 support...
        destinationBus->zero();
    }
}

void Reverb::reset()
{
    for (size_t i = 0; i < m_convolvers.size(); ++i)
        m_convolvers[i]->reset();
}

size_t Reverb::latencyFrames() const
{
    return !m_convolvers.empty() ? (*m_convolvers.begin())->latencyFrames() : 0;
}

} // namespace lab
