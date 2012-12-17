/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioDestinationMac.h"

#include "AudioIOCallback.h"
#include "FloatConversion.h"
#include "VectorMath.h"
#include <CoreAudio/AudioHardware.h>

namespace WebCore {

const int kBufferSize = 128;
const float kLowThreshold = -1;
const float kHighThreshold = 1;
    
//LabSound start
class AudioDestinationMac::Input
{
public:
    Input()
    : m_inputUnit(0)
    , m_buffers(0)
    , m_audioBus(0)
    {
        AudioComponent comp;
        AudioComponentDescription desc;
        desc.componentType = kAudioUnitType_Output;
#if TARGET_OS_IPHONE
        desc.componentSubType = kAudioUnitSubType_RemoteIO;
#else
        desc.componentSubType = kAudioUnitSubType_HALOutput;
#endif
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        comp = AudioComponentFindNext(0, &desc);

        ASSERT(comp);
        
        OSStatus result = AudioComponentInstanceNew(comp, &m_inputUnit);
        ASSERT(!result);
        
        result = AudioUnitInitialize(m_inputUnit);
        ASSERT(!result);
    }
    
    ~Input()
    {
        if (m_inputUnit)
            AudioComponentInstanceDispose(m_inputUnit);
        
        free(m_buffers);

        if (m_audioBus)
            delete m_audioBus;
        
    }
    
    void configure(const AudioStreamBasicDescription& outDesc, UInt32 bufferSize)
    {
        // enable IO on input
        UInt32 param = 1;
        OSErr result = AudioUnitSetProperty(m_inputUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &param, sizeof(UInt32));
        ASSERT(!result);
        
        // disable IO on output
        param = 0;
        result = AudioUnitSetProperty(m_inputUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &param, sizeof(UInt32));
        ASSERT(!result);
        
#if !TARGET_OS_IPHONE
        // set to use default device
        AudioDeviceID deviceId = kAudioObjectUnknown;
        param = sizeof(AudioDeviceID);
        AudioObjectPropertyAddress property_address = {
            kAudioHardwarePropertyDefaultInputDevice,  // mSelector
            kAudioObjectPropertyScopeGlobal,            // mScope
            kAudioObjectPropertyElementMaster           // mElement
        };
        UInt32 deviceIdSize = sizeof(deviceId);
        result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                            &property_address,
                                            0,     // inQualifierDataSize
                                            NULL,  // inQualifierData
                                            &deviceIdSize,
                                            &deviceId);
        ASSERT(!result);
        
        result = AudioUnitSetProperty(m_inputUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &deviceId, sizeof(AudioDeviceID));
        ASSERT(!result);
#endif
        
        // configure the callback
        AURenderCallbackStruct callback;
        callback.inputProc = inputCallback;
        callback.inputProcRefCon = this;
        result = AudioUnitSetProperty(m_inputUnit, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 0, &callback, sizeof(AURenderCallbackStruct));
        ASSERT(!result);
        
        // make the input buffer size match the output buffer size
        UInt32 bufferSizeVal = bufferSize;
#if TARGET_OS_IPHONE
        result = AudioUnitSetProperty(m_inputUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &bufferSizeVal, sizeof(bufferSizeVal));
#else
        result = AudioUnitSetProperty(m_inputUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &bufferSizeVal, sizeof(bufferSizeVal));
#endif
        ASSERT(!result);
        
        // Initialize the AudioUnit
        result = AudioUnitInitialize(m_inputUnit);
        ASSERT(!result);
        
        // get Size of IO Buffers
        UInt32 sampleCount;
        param = sizeof(UInt32);
#if TARGET_OS_IPHONE
        result = AudioUnitGetProperty(m_inputUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &sampleCount, &param);
#else
        result = AudioUnitGetProperty(m_inputUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &sampleCount, &param);
#endif
        ASSERT(!result);
        
        // The AudioUnit can do format conversions, so match the input configuration to the output.
        //// if this doesn't work try it the other way around - set up the input desc and force the output to match
        param = sizeof(AudioStreamBasicDescription);
        result = AudioUnitSetProperty(m_inputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &outDesc, param);
        ASSERT(!result);
        
        m_audioBus = new AudioBus(2, bufferSize, true);
        
        m_buffers = (AudioBufferList*) malloc(offsetof(AudioBufferList, mBuffers[0]) + sizeof(AudioBuffer) * outDesc.mChannelsPerFrame);
        m_buffers->mNumberBuffers = outDesc.mChannelsPerFrame;
        for (int i = 0; i < m_buffers->mNumberBuffers; ++i) {
            m_buffers->mBuffers[i].mNumberChannels = 1;
            m_buffers->mBuffers[i].mDataByteSize = bufferSize * outDesc.mBytesPerFrame;
            m_buffers->mBuffers[i].mData = m_audioBus->channel(i)->mutableData();
        }
    }
    
    static OSStatus inputCallback(void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
    {
        Input* input = reinterpret_cast<Input*>( inRefCon );
        OSStatus result = AudioUnitRender(input->m_inputUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, input->m_buffers);
        
        if (result != noErr)
            for (int i = 0; i < input->m_buffers->mNumberBuffers; ++i)
                input->m_audioBus->channel(i)->zero();
        
        return noErr;
    }
    
    AudioUnit m_inputUnit;
    AudioBufferList* m_buffers;
    AudioBus* m_audioBus;
};
//LabSound end

// Factory method: Mac-implementation
PassOwnPtr<AudioDestination> AudioDestination::create(AudioIOCallback& callback, float sampleRate)
{
    return adoptPtr(new AudioDestinationMac(callback, sampleRate));
}

float AudioDestination::hardwareSampleRate()
{
    // Determine the default output device's sample-rate.
    AudioDeviceID deviceID = kAudioDeviceUnknown;
    UInt32 infoSize = sizeof(deviceID);

    AudioObjectPropertyAddress defaultOutputDeviceAddress = { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    OSStatus result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &defaultOutputDeviceAddress, 0, 0, &infoSize, (void*)&deviceID);
    if (result)
        return 0; // error

    Float64 nominalSampleRate;
    infoSize = sizeof(Float64);

    AudioObjectPropertyAddress nominalSampleRateAddress = { kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    result = AudioObjectGetPropertyData(deviceID, &nominalSampleRateAddress, 0, 0, &infoSize, (void*)&nominalSampleRate);
    if (result)
        return 0; // error

    return narrowPrecisionToFloat(nominalSampleRate);
}

AudioDestinationMac::AudioDestinationMac(AudioIOCallback& callback, float sampleRate)
    : m_outputUnit(0)
    , m_callback(callback)
    , m_renderBus(2, kBufferSize, false)
    , m_sampleRate(sampleRate)
    , m_isPlaying(false)
    , m_input(new Input()) // LabSound
{
    // Open and initialize DefaultOutputUnit
    AudioComponent comp;
    AudioComponentDescription desc;

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    comp = AudioComponentFindNext(0, &desc);

    ASSERT(comp);

    OSStatus result = AudioComponentInstanceNew(comp, &m_outputUnit);
    ASSERT(!result);

    result = AudioUnitInitialize(m_outputUnit);
    ASSERT(!result);

    configure();
}

AudioDestinationMac::~AudioDestinationMac()
{
    if (m_outputUnit)
        AudioComponentInstanceDispose(m_outputUnit);
    
    delete m_input; // LabSound
}

void AudioDestinationMac::configure()
{
    // Set render callback
    AURenderCallbackStruct input;
    input.inputProc = inputProc;
    input.inputProcRefCon = this;
    OSStatus result = AudioUnitSetProperty(m_outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, 0, &input, sizeof(input));
    ASSERT(!result);

    // Set stream format
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate = m_sampleRate;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagsCanonical | kAudioFormatFlagIsNonInterleaved;
    streamFormat.mBitsPerChannel = 8 * sizeof(AudioSampleType);
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = sizeof(AudioSampleType);
    streamFormat.mBytesPerFrame = sizeof(AudioSampleType);

    result = AudioUnitSetProperty(m_outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, (void*)&streamFormat, sizeof(AudioStreamBasicDescription));
    ASSERT(!result);

    // Set the buffer frame size.
    UInt32 bufferSize = kBufferSize;
    result = AudioUnitSetProperty(m_outputUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Output, 0, (void*)&bufferSize, sizeof(bufferSize));
    ASSERT(!result);
    
    m_input->configure(streamFormat, bufferSize);
}

void AudioDestinationMac::start()
{
    OSStatus result = AudioOutputUnitStart(m_outputUnit);

    if (!result)
        m_isPlaying = true;
    
    // LabSound
    result = AudioOutputUnitStart(m_input->m_inputUnit);
}

void AudioDestinationMac::stop()
{
    OSStatus result = AudioOutputUnitStop(m_outputUnit);

    if (!result)
        m_isPlaying = false;

    // LabSound
    result = AudioOutputUnitStop(m_input->m_inputUnit);
}

// Pulls on our provider to get rendered audio stream.
OSStatus AudioDestinationMac::render(UInt32 numberOfFrames, AudioBufferList* ioData)
{
    AudioBuffer* buffers = ioData->mBuffers;
    m_renderBus.setChannelMemory(0, (float*)buffers[0].mData, numberOfFrames);
    m_renderBus.setChannelMemory(1, (float*)buffers[1].mData, numberOfFrames);

    // FIXME: Add support for local/live audio input.
    // LabSound m_callback.render(0, &m_renderBus, numberOfFrames);
    m_callback.render(m_input->m_audioBus, &m_renderBus, numberOfFrames); // LabSound

    // Clamp values at 0db (i.e., [-1.0, 1.0])
    for (unsigned i = 0; i < m_renderBus.numberOfChannels(); ++i) {
        AudioChannel* channel = m_renderBus.channel(i);
        VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
    }

    return noErr;
}

// DefaultOutputUnit callback
OSStatus AudioDestinationMac::inputProc(void* userData, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32 /*busNumber*/, UInt32 numberOfFrames, AudioBufferList* ioData)
{
    AudioDestinationMac* audioOutput = static_cast<AudioDestinationMac*>(userData);
    return audioOutput->render(numberOfFrames, ioData);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
