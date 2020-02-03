// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioHardwareDeviceNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSourceProvider.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"
#include "internal/DenormalDisabler.h"

#include <chrono>

using namespace lab;

/////////////////////////////////////////////////////
//   AudioHardwareDeviceNode::AudioHardwareInput   //
/////////////////////////////////////////////////////

// AudioHardwareInput allows us to expose an AudioSourceProvider for local/live audio input.
// If there is local/live audio input, we call set() with the audio input data every render quantum.
// `set()` is called in AudioHardwareDeviceNode::render, which is one or two frames above the actual hardware io.
class AudioHardwareDeviceNode::AudioHardwareInput : public AudioSourceProvider
{
    AudioBus m_sourceBus;

public:
    AudioHardwareInput(size_t channelCount)
        : m_sourceBus(channelCount, AudioNode::ProcessingSizeInFrames)
    {
        epoch[0] = epoch[1] = std::chrono::high_resolution_clock::now();
    }

    virtual ~AudioHardwareInput() {}

    void set(AudioBus * bus)
    {
        if (bus) m_sourceBus.copyFrom(*bus);
    }

    // Satisfy the AudioSourceProvider interface
    virtual void provideInput(AudioBus * destinationBus, size_t numberOfFrames)
    {
        bool isGood = destinationBus && destinationBus->length() == numberOfFrames && m_sourceBus.length() == numberOfFrames;
        ASSERT(isGood);
        if (isGood) destinationBus->copyFrom(m_sourceBus);
    }

    // low bit of m_currentSampleFrame indexes time point 0 or 1
    // this is so that time and epoch are written atomically, after the alternative epoch has been filled in.
    std::chrono::high_resolution_clock::time_point epoch[2];
};

/////////////////////////////////
//   AudioHardwareDeviceNode   //
/////////////////////////////////

AudioHardwareDeviceNode::AudioHardwareDeviceNode(AudioContext * context, const AudioStreamConfig outputConfig, const AudioStreamConfig inputConfig)
    : m_context(context)
{
    // Ensure that input and output samplerates match
    if (inputConfig.device_index != -1)
    {
        ASSERT(outputConfig.desired_samplerate == inputConfig.desired_samplerate);
    }

    m_destination = std::unique_ptr<AudioDevice>(AudioDevice::MakePlatformSpecificDevice(*this, outputConfig, inputConfig));

    LOG("MakePlatformSpecificDevice() \n"
        "\tSample Rate:     %f \n"
        "\tInput Channels:  %i \n"
        "\tOutput Channels: %i \n",
        outputConfig.desired_samplerate, inputConfig.desired_channels, outputConfig.desired_channels);

    if (inputConfig.device_index != -1)
    {
        m_audioHardwareInput = new AudioHardwareInput(inputConfig.desired_channels);  // @fixme
        m_audioHardwareInput->epoch[0] = m_audioHardwareInput->epoch[1] = std::chrono::high_resolution_clock::now();
    }

    // This is the final node in the chain. It willpull on all others from this input. 
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));

    // Node-specific default mixing rules.
    //m_channelCount = outputConfig.desired_channels;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Speakers;
    
    ContextGraphLock glock(context, "AudioHardwareDeviceNode");

    AudioNode::setChannelCount(glock, outputConfig.desired_channels);

    // NB: Special case - the audio context calls initialize so that rendering doesn't start before the context is ready
    initialize();
}

AudioHardwareDeviceNode::~AudioHardwareDeviceNode()
{
    uninitialize();
}

void AudioHardwareDeviceNode::initialize()
{
    if (isInitialized()) return;
    AudioNode::initialize();
}

void AudioHardwareDeviceNode::uninitialize()
{
    if (!isInitialized()) return;
    m_destination->stop();
    AudioNode::uninitialize();
}

void AudioHardwareDeviceNode::start()
{
    initialize(); // presumably called by the context

    if (isInitialized())
    {
        m_destination->start();
    }
}

void AudioHardwareDeviceNode::stop()
{
    m_destination->stop();
}

void AudioHardwareDeviceNode::reset(ContextRenderLock &)
{
    m_currentSampleFrame = 0;
};

void AudioHardwareDeviceNode::render(AudioBus * sourceBus, AudioBus * destinationBus, size_t numberOfFrames)
{
    // The audio system might still be invoking callbacks during shutdown, so bail out if so.
    if (!m_context) return;

    ContextRenderLock renderLock(m_context, "AudioHardwareDeviceNode::render");
    if (!renderLock.context()) return;  // return if couldn't acquire lock

    if (!m_context->isInitialized())
    {
        destinationBus->zero();
        return;
    }

    // Denormals can slow down audio processing.
    /// @TODO under what circumstance do they arise?
    /// If they come from input data such as loaded WAV files, they should be corrected
    /// at source. If they can result from signal processing; again, where? The
    /// signal processing should not produce denormalized values.
    /// Use an RAII object to protect all AudioNodes processed within this scope.
    DenormalDisabler denormalDisabler;

    // Let the context take care of any business at the start of each render quantum.
    m_context->handlePreRenderTasks(renderLock);

    // Prepare the local audio input provider for this render quantum.
    if (sourceBus)
    {
        m_audioHardwareInput->set(sourceBus);
    }

    // process the graph by pulling the inputs, which will recurse the entire processing graph.
    AudioBus * renderedBus = input(0)->pull(renderLock, destinationBus, numberOfFrames);

    if (!renderedBus)
    {
        destinationBus->zero();
    }
    else if (renderedBus != destinationBus)
    {
        // in-place processing was not possible - so copy
        destinationBus->copyFrom(*renderedBus);
    }

    // Process nodes which need a little extra help because they are not connected to anything, but still need to process.
    m_context->processAutomaticPullNodes(renderLock, numberOfFrames);

    // Let the context take care of any business at the end of each render quantum.
    m_context->handlePostRenderTasks(renderLock);

    // Advance current sample-frame.
    int index = 1 - (m_currentSampleFrame & 1);
    uint64_t t = m_currentSampleFrame & ~1;
    m_currentSampleFrame = t + numberOfFrames * 2 + index;

    if (m_audioHardwareInput) m_audioHardwareInput->epoch[index] = std::chrono::high_resolution_clock::now();
}

uint64_t AudioHardwareDeviceNode::currentSampleFrame() const
{
    return m_currentSampleFrame / 2;  /// @TODO why div 2?
}

double AudioHardwareDeviceNode::currentTime() const
{
    return currentSampleFrame() / static_cast<double>(m_destination->getSampleRate());
}

double AudioHardwareDeviceNode::currentSampleTime() const
{
    uint64_t t = m_currentSampleFrame;
    int index = t & 1;
    double val = (t / 2) / static_cast<double>(m_destination->getSampleRate());
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point::duration dt = t2 - m_audioHardwareInput->epoch[index];
    return val + (std::chrono::duration_cast<std::chrono::microseconds>(dt).count() * 1.0e-6f);
}

AudioSourceProvider * AudioHardwareDeviceNode::AudioHardwareInputProvider()
{
    return static_cast<AudioSourceProvider *>(m_audioHardwareInput);
}