// License: BSD 3 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioChannel_h
#define AudioChannel_h

#include "LabSound/core/AudioArray.h"

namespace lab
{


// An AudioChannel represents a buffer of non-interleaved floating-point audio samples.
// The PCM samples are normally assumed to be in a nominal range -1.0 -> +1.0
class AudioChannel
{
    AudioChannel(const AudioChannel &) = delete;  // noncopyable

public:
    explicit AudioChannel(int length)
        : m_length(length)
        , m_silent(true)
    {
        m_memBuffer = new AudioFloatArray(length);
    }

    explicit AudioChannel() = delete;

    ~AudioChannel() {
        delete m_memBuffer;
    }

    // How many sample-frames do we contain?
    int length() const { return m_length; }

    // Direct access to PCM sample data. Non-const accessor clears silent flag.
    float * mutableData()
    {
        clearSilentFlag();
        return const_cast<float *>(data());
    }

    const float * data() const
    {
        if (m_memBuffer)
            return m_memBuffer->data();
        return nullptr;
    }

    // Zeroes out all sample values in buffer.
    void zero()
    {
        if (m_silent) return;

        m_silent = true;

        if (m_memBuffer)
            m_memBuffer->zero();
    }

    // Clears the silent flag.
    void clearSilentFlag() { m_silent = false; }

    bool isSilent() const { return m_silent; }

    // Scales all samples by the same amount.
    void scale(float scale);

    // A simple memcpy() from the source channel
    void copyFrom(const AudioChannel * sourceChannel);

    // Copies the given range from the source channel.
    void copyFromRange(const AudioChannel * sourceChannel, int startFrame, int endFrame);

    // Sums (with unity gain) from the source channel.
    void sumFrom(const AudioChannel * sourceChannel);

    // Returns maximum absolute value (useful for normalization).
    float maxAbsValue() const;

private:
    int m_length = 0;
    AudioFloatArray* m_memBuffer;
    bool m_silent = true;
};

}  // lab

#endif  // AudioChannel_h
