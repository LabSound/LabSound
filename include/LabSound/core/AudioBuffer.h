// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioBuffer_h
#define AudioBuffer_h

#include <vector>
#include <memory>

namespace lab
{

class AudioBus;
class AudioBuffer;

class AudioBuffer 
{

public:   

    AudioBuffer(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate);

    explicit AudioBuffer(AudioBus*);

    ~AudioBuffer();

    // Format
    size_t length() const { return m_length; }
    double duration() const { return length() / sampleRate(); }
    float sampleRate() const { return m_sampleRate; }

    // Channel data access
    unsigned numberOfChannels() const { return (unsigned) m_channels.size(); }
    std::shared_ptr<std::vector<float>> getChannelData(unsigned channelIndex);
    void zero();

    // Scalar gain
    double gain() const { return m_gain; }
    void setGain(double gain) { m_gain = gain; }

    void releaseMemory();
    
protected:

    double m_gain; // scalar gain
    float m_sampleRate;
    size_t m_length;

    std::vector< std::shared_ptr<std::vector<float> > > m_channels;
};

} // namespace lab

#endif // AudioBuffer_h
