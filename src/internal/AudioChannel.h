// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioChannel_h
#define AudioChannel_h

#include "LabSound/core/AudioArray.h"

#include <memory>

namespace lab {

// An AudioChannel represents a buffer of non-interleaved floating-point audio samples.
// The PCM samples are normally assumed to be in a nominal range -1.0 -> +1.0
class AudioChannel {
    AudioChannel(const AudioChannel&); // noncopyable
public:
    // Memory can be externally referenced, or can be internally allocated with an AudioFloatArray.

    // Reference an external buffer.
    AudioChannel(float* storage, size_t length)
        : m_length(length)
        , m_rawPointer(storage)
        , m_silent(false)
    {
    }

    // Manage storage for us.
    explicit AudioChannel(size_t length)
        : m_length(length)
        , m_silent(true)
    {
        m_memBuffer.reset(new AudioFloatArray(length));
    }

    // A "blank" audio channel -- must call set() before it's useful...
    AudioChannel()
        : m_length(0)
        , m_silent(true)
    {
    }

    // Redefine the memory for this channel.
    // storage represents external memory not managed by this object.
    void set(float* storage, size_t length)
    {
        m_memBuffer.reset();//  .clear(); // cleanup managed storage
        m_rawPointer = storage;
        m_length = length;
        m_silent = false;
    }

    // How many sample-frames do we contain?
    size_t length() const { return m_length; }

    // resizeSmaller() can only be called with a new length <= the current length.
    // The data stored in the bus will remain undisturbed.
    void resizeSmaller(size_t newLength);

    // Direct access to PCM sample data. Non-const accessor clears silent flag.
    float * mutableData()
    {
        clearSilentFlag();
        return m_rawPointer ? m_rawPointer : m_memBuffer->data(); 
    }

    const float* data() const { return m_rawPointer ? m_rawPointer : m_memBuffer->data(); }

    // Zeroes out all sample values in buffer.
    void zero()
    {
        if (m_silent)
            return;

        m_silent = true;

        if (m_memBuffer.get())
            m_memBuffer->zero();
        else
            memset(m_rawPointer, 0, sizeof(float) * m_length);
    }

    // Clears the silent flag.
    void clearSilentFlag() { m_silent = false; }

    bool isSilent() const { return m_silent; }

    // Scales all samples by the same amount.
    void scale(float scale);

    // A simple memcpy() from the source channel
    void copyFrom(const AudioChannel* sourceChannel);

    // Copies the given range from the source channel.
    void copyFromRange(const AudioChannel* sourceChannel, unsigned startFrame, unsigned endFrame);

    // Sums (with unity gain) from the source channel.
    void sumFrom(const AudioChannel* sourceChannel);

    // Returns maximum absolute value (useful for normalization).
    float maxAbsValue() const;

private:

    size_t m_length;
    float * m_rawPointer = nullptr;
    std::unique_ptr<AudioFloatArray> m_memBuffer;
    bool m_silent;
};

} // lab

#endif // AudioChannel_h
