// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioBus.h"

#include "LabSound/extended/RecorderNode.h"
#include "internal/Assertions.h"

#include "libnyquist/Encoders.h"

using namespace lab;
    
RecorderNode::RecorderNode(const AudioStreamConfig outConfig)
    : outConfig(outConfig), AudioBasicInspectorNode(outConfig.desired_channels)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, outConfig.desired_channels)));

    m_channelCount = outConfig.desired_channels;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Discrete;

    initialize();
}
    
RecorderNode::~RecorderNode()
{
    uninitialize();
}

void RecorderNode::process(ContextRenderLock & r, size_t framesToProcess)
{
    AudioBus * outputBus = output(0)->bus(r);
        
    if (!isInitialized() || !input(0)->isConnected())
    {
        if (outputBus) 
        {
            outputBus->zero();
        }
        return;
    }

    // =====> should this follow the WebAudio pattern have a writer object to call here?
    AudioBus * bus = input(0)->bus(r);        
    bool isBusGood = bus && (bus->numberOfChannels() > 0) && (bus->channel(0)->length() >= framesToProcess);
        
    ASSERT(isBusGood);
    if (!isBusGood)
    {
        outputBus->zero();
        return;
    }
        
    if (m_recording)
    {
        std::vector<const float*> channels;
        const size_t inputBusNumChannels = bus->numberOfChannels();

        for (size_t i = 0; i < inputBusNumChannels; ++i)
        {
            channels.push_back(bus->channel(i)->data());
        }

        // mix down the output, or interleave the output
        // use the tightest loop possible since this is part of the processing step
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
            
        m_data.reserve(framesToProcess * (m_mixToMono ? 1 : inputBusNumChannels)); 
            
        if (m_mixToMono)
        {
            if (inputBusNumChannels == Channels::Mono)
            {
                for (size_t i = 0; i < framesToProcess; ++i)
                {
                    m_data.push_back(channels[0][i]);
                }
            }
            else
            {
                for (size_t i = 0; i < framesToProcess; ++i)
                {
                    float val = 0;
                    for (size_t c = 0; c < inputBusNumChannels; ++c)
                    {
                        val += channels[c][i];
                    }
                    val *= 1.f / float(inputBusNumChannels);
                    m_data.push_back(val);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < framesToProcess; ++i)
            {   
                for (size_t c = 0; c < inputBusNumChannels; ++ c)
                {
                    m_data.push_back(channels[c][i]);
                }
            }
        }
    }
    // <====== to here
        
    // For in-place processing, our override of pullInputs() will just pass the audio data
    // through unchanged if the channel count matches from input to output
    // (resulting in inputBus == outputBus). Otherwise, do an up-mix to stereo.
    if (bus != outputBus)
    {
        outputBus->copyFrom(*bus);
    }
}
    
void RecorderNode::writeRecordingToWav(const std::string & filenameWithWavExtension)
{
    std::unique_ptr<nqr::AudioData> fileData(new nqr::AudioData());
        
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        fileData->samples.swap(m_data);
    }
        
    fileData->sampleRate   = static_cast<int>(outConfig.desired_samplerate);
    fileData->channelCount = outConfig.desired_channels;
    fileData->sourceFormat = nqr::PCM_FLT;

    nqr::EncoderParams params = {static_cast<int>(outConfig.desired_channels), nqr::PCM_FLT, nqr::DITHER_NONE};

    const int encoder_status = nqr::encode_wav_to_disk(params, fileData.get(), filenameWithWavExtension);
}
    
void RecorderNode::reset(ContextRenderLock & r)
{
    std::vector<float> clear;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_data.swap(clear);
    }
}
                                           
