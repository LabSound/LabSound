// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioBus_h
#define AudioBus_h

#include "LabSound/core/Mixing.h"
#include "LabSound/core/AudioChannel.h"
#include "LabSound/core/AudioArray.h"
#include <vector>
#include <iostream>

namespace lab {

    using lab::ChannelInterpretation;
    using lab::Channel;

// An AudioBus represents a collection of one or more AudioChannels.
// The data layout is "planar" as opposed to "interleaved".
// An AudioBus with one channel is mono, an AudioBus with two channels is stereo, etc.
class AudioBus
{
    AudioBus(AudioBus &) = delete;

public:
    // clone an existing audio bus. If the source bus references external memory
    // the clone will not, it will take a copy of what was there at the time the
    // clone was made.
    AudioBus(const AudioBus &);
    explicit AudioBus(size_t numberOfChannels, size_t length);

    // Channels
    uint32_t numberOfChannels() const { return static_cast<uint32_t>(m_channels.size()); }

    // Use this when looping over channels
    AudioChannel * channel(size_t channel)
    {
        if (channel >= m_channels.size())
            return nullptr;

        return m_channels[channel].get();
    }

    const AudioChannel * channel(size_t channel) const
    {
        return const_cast<AudioBus*>(this)->m_channels[channel].get();
    }

    // use this when accessing channels semantically
    AudioChannel* channelByType(Channel type);
    const AudioChannel* channelByType(Channel type) const;

    // Number of sample-frames
    size_t length() const { return m_length; }

    // setLength preserves existing data
    void setLength(size_t newLength);

    // Sample-rate : 0.0 if unknown or "don't care"
    float sampleRate() const { return m_sampleRate; }
    void setSampleRate(float sampleRate) { m_sampleRate = sampleRate; }

    // Zeroes all channels.
    void zero();

    // Clears the silent flag on all channels.
    void clearSilentFlag();

    // Returns true if the silent bit is set on all channels.
    bool isSilent() const;

    // Returns true if the channel count and frame-size match.
    bool topologyMatches(const AudioBus &sourceBus) const;

    // Creates a new AudioBus by sample-rate converting sourceBus to the newSampleRate.
    // setSampleRate() must have been previously called on sourceBus.
    // Note: sample-rate conversion is already handled in the file-reading code for the mac port, so we don't need this.
    static std::unique_ptr<AudioBus> createBySampleRateConverting(const AudioBus * sourceBus, bool mixToMono, float newSampleRate);

    // Creates a new AudioBus by mixing all the channels down to mono.
    // If sourceBus is already mono, then the returned AudioBus will simply be a copy.
    static std::unique_ptr<AudioBus> createByMixingToMono(const AudioBus* sourceBus);

    // Scales all samples by the same amount.
    void scale(float scale);

    void reset() { m_isFirstTime = true; } // for de-zippering

    // Assuming sourceBus has the same topology, copies sample data from each channel of sourceBus to our corresponding channel.
    void copyFrom(const AudioBus & sourceBus, ChannelInterpretation = ChannelInterpretation::Speakers);

    // Sums the sourceBus into our bus with unity gain.
    // Our own internal gain m_busGain is ignored.
    void sumFrom(const AudioBus & sourceBus, ChannelInterpretation = ChannelInterpretation::Speakers);

    // Copy each channel from sourceBus into our corresponding channel.
    // We scale by targetGain (and our own internal gain m_busGain), performing "de-zippering" to smoothly change from *lastMixGain to (targetGain*m_busGain).
    // The caller is responsible for setting up lastMixGain to point to storage which is unique for every "stream" which will be applied to this bus.
    // This represents the dezippering memory.
    void copyWithGainFrom(const AudioBus &sourceBus, float* lastMixGain, float targetGain);

    // Copies the sourceBus by scaling with sample-accurate gain values.
    void copyWithSampleAccurateGainValuesFrom(const AudioBus & sourceBus, float* gainValues, size_t numberOfGainValues);

    // Returns maximum absolute value across all channels (useful for normalization).
    float maxAbsValue() const;

    // Makes maximum absolute value == 1.0 (if possible).
    void normalize();

    bool isFirstTime() { return m_isFirstTime; }

protected:

    AudioBus() = default;

    void speakersCopyFrom(const AudioBus&);
    void discreteCopyFrom(const AudioBus&);
    void speakersSumFrom(const AudioBus&);
    void discreteSumFrom(const AudioBus&);
    void speakersSumFrom5_1_ToMono(const AudioBus&);
    void speakersSumFrom7_1_ToMono(const AudioBus&);

    size_t m_length = 0;
    float m_busGain = 1.0f;
    bool m_isFirstTime = true;
    float m_sampleRate = 0.0f;

    std::vector<std::unique_ptr<AudioChannel>> m_channels;
    std::unique_ptr<AudioFloatArray> m_dezipperGainValues;
};

} // lab

#endif // AudioBus_h
