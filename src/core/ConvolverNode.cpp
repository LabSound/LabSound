// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/ConvolverNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/Reverb.h"

using namespace std;

// Note about empirical tuning:
// The maximum FFT size affects reverb performance and accuracy.
// If the reverb is single-threaded and processes entirely in the real-time audio thread,
// it's important not to make this too high.  In this case 8192 is a good value.
// But, the Reverb object is multi-threaded, so we want this as high as possible without losing too much accuracy.
// Very large FFTs will have worse phase errors. Given these constraints 32768 is a good compromise.
const size_t MaxFFTSize = 32768;

namespace lab {

ConvolverNode::ConvolverNode() : m_swapOnRender(false)
, m_normalize(std::make_shared<AudioSetting>("normalize"))
{
    addInput(unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 2)));
    
    // Node-specific default mixing rules.
    m_channelCount = 2;
    m_channelCountMode = ChannelCountMode::ClampedMax;
    m_channelInterpretation = ChannelInterpretation::Speakers;

    m_settings.push_back(m_normalize);
    
    initialize();
}

ConvolverNode::~ConvolverNode()
{
    uninitialize();
}

void ConvolverNode::process(ContextRenderLock & r, size_t framesToProcess)
{
    if (m_swapOnRender)
    {
        m_reverb = std::move(m_newReverb);
        m_bus = m_newBus;
        m_newBus.reset();
        m_swapOnRender = false;
    }
    
    AudioBus * outputBus = output(0)->bus(r);
    
    if (!isInitialized() || !m_reverb)
    {
        if (outputBus) outputBus->zero();
        return;
    }

    // Process using the convolution engine.
    // Note that we can handle the case where nothing is connected to the input, in which case we'll just feed silence into the convolver.
    // FIXME: If we wanted to get fancy we could try to factor in the 'tail time' and stop processing once the tail dies down if
    // we keep getting fed silence.
    m_reverb->process(r, input(0)->bus(r), outputBus, framesToProcess);
}

void ConvolverNode::reset(ContextRenderLock&)
{
    m_newReverb.reset();
    m_swapOnRender = true;
}

void ConvolverNode::initialize()
{
    if (isInitialized())
        return;
        
    AudioNode::initialize();
}

void ConvolverNode::uninitialize()
{
    m_reverb.reset();

    if (!isInitialized())
        return;

    AudioNode::uninitialize();
}

void ConvolverNode::setImpulse(std::shared_ptr<AudioBus> bus)
{
    if (!bus) return;

    size_t numberOfChannels = bus->numberOfChannels();
    size_t bufferLength = bus->length();

    // The current implementation supports up to four channel impulse responses, which are interpreted as true-stereo (see Reverb class).
    bool isBufferGood = numberOfChannels > 0 && numberOfChannels <= 4 && bufferLength;
    ASSERT(isBufferGood);
    if (!isBufferGood) return;

    // Create the reverb with the given impulse response.
    const bool threaded = false;
    m_newReverb = std::unique_ptr<Reverb>(new Reverb(bus.get(), AudioNode::ProcessingSizeInFrames, MaxFFTSize, 2, threaded, normalize()));
    m_newBus = bus;
    m_swapOnRender = true;
}

std::shared_ptr<AudioBus> ConvolverNode::getImpulse()
{
    if (m_swapOnRender) return m_newBus;
    return m_bus;
}

double ConvolverNode::tailTime(ContextRenderLock & r) const
{
    return m_reverb ? m_reverb->impulseResponseLength() / static_cast<double>(r.context()->sampleRate()) : 0;
}

double ConvolverNode::latencyTime(ContextRenderLock & r) const
{
    return m_reverb ? m_reverb->latencyFrames() / static_cast<double>(r.context()->sampleRate()) : 0;
}

bool ConvolverNode::normalize() const
{
    return m_normalize->valueUint32() != 0;
}
void ConvolverNode::setNormalize(bool normalize)
{
    m_normalize->setUint32(normalize ? 1 : 0);
}


} // namespace lab
