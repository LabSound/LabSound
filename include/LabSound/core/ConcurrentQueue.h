// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#include <cstdlib>
#include <cstring>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace lab {

// Ringbuffer (aka circular buffer) data structure for use in concurrent audio scenarios.
// Other than minor modifications, this ringbuffer is a copy of Tim Blechmann's fine work, found as the base
// structure of boost::lockfree::spsc_queue (ringbuffer_base). Whereas the boost::lockfree data structures
// are meant for a wide range of applications / archs, this version specifically caters to audio processing.
// The implementation remains lock-free and thread-safe within a single write thread / single read thread context.
// Copyright (c) 2014, The Cinder Project
// https://github.com/cinder/Cinder/blob/master/COPYING

template <typename T>
class RingBufferT
{
public:
    // Constructs a RingBufferT with size = 0
    RingBufferT()
        : mData(nullptr)
        , mAllocatedSize(0)
        , mWriteIndex(0)
        , mReadIndex(0)
    {
    }

    // Constructs a RingBufferT with \a count maximum elements.
    RingBufferT(size_t count)
        : mAllocatedSize(0)
    {
        resize(count);
    }

    RingBufferT(RingBufferT && other)
        : mData(other.mData)
        , mAllocatedSize(other.mAllocatedSize)
        , mWriteIndex(0)
        , mReadIndex(0)
    {
        other.mData = nullptr;
        other.mAllocatedSize = 0;
    }

    ~RingBufferT()
    {
        if (mData) free(mData);
    }

    // Resizes the container to contain \a count maximum elements. Invalidates the internal buffer and resets read / write indices to 0. \note Must be synchronized with both read and write threads.
    void resize(size_t count)
    {
        size_t allocatedSize = count + 1;  // one bin is used to distinguish between the read and write indices when full.

        if (mAllocatedSize)
            mData = (T *) std::realloc(mData, allocatedSize * sizeof(T));
        else
            mData = (T *) std::calloc(allocatedSize, sizeof(T));

        assert(mData);

        mAllocatedSize = allocatedSize;
        clear();
    }

    // Invalidates the internal buffer and resets read / write indices to 0. Must be synchronized with both read and write threads.
    void clear()
    {
        mWriteIndex = 0;
        mReadIndex = 0;
    }

    // Returns the maximum number of elements.
    size_t getSize() const
    {
        return mAllocatedSize - 1;
    }

    // Returns the number of elements available for writing. Only safe to call from the write thread.
    size_t getAvailableWrite() const
    {
        return getAvailableWrite(mWriteIndex, mReadIndex);
    }

    // Returns the number of elements available for reading. Only safe to call from the read thread.
    size_t getAvailableRead() const
    {
        return getAvailableRead(mWriteIndex, mReadIndex);
    }

    // Only safe to call from the write thread.
    bool write(const T * array, size_t count)
    {
        const size_t writeIndex = mWriteIndex.load(std::memory_order_relaxed);
        const size_t readIndex = mReadIndex.load(std::memory_order_acquire);

        if (count > getAvailableWrite(writeIndex, readIndex))
            return false;

        size_t writeIndexAfter = writeIndex + count;

        if (writeIndex + count > mAllocatedSize)
        {
            size_t countA = mAllocatedSize - writeIndex;
            size_t countB = count - countA;

            std::memcpy(mData + writeIndex, array, countA * sizeof(T));
            std::memcpy(mData, array + countA, countB * sizeof(T));
            writeIndexAfter -= mAllocatedSize;
        }
        else
        {
            std::memcpy(mData + writeIndex, array, count * sizeof(T));
            if (writeIndexAfter == mAllocatedSize)
                writeIndexAfter = 0;
        }

        mWriteIndex.store(writeIndexAfter, std::memory_order_release);
        return true;
    }

    // Only safe to call from the read thread.
    bool read(T * array, size_t count)
    {
        const size_t writeIndex = mWriteIndex.load(std::memory_order_acquire);
        const size_t readIndex = mReadIndex.load(std::memory_order_relaxed);

        if (count > getAvailableRead(writeIndex, readIndex))
            return false;

        size_t readIndexAfter = readIndex + count;

        if (readIndex + count > mAllocatedSize)
        {
            size_t countA = mAllocatedSize - readIndex;
            size_t countB = count - countA;

            std::memcpy(array, mData + readIndex, countA * sizeof(T));
            std::memcpy(array + countA, mData, countB * sizeof(T));

            readIndexAfter -= mAllocatedSize;
        }
        else
        {
            std::memcpy(array, mData + readIndex, count * sizeof(T));
            if (readIndexAfter == mAllocatedSize)
                readIndexAfter = 0;
        }

        mReadIndex.store(readIndexAfter, std::memory_order_release);
        return true;
    }

private:
    size_t getAvailableWrite(size_t writeIndex, size_t readIndex) const
    {
        size_t result = readIndex - writeIndex - 1;
        if (writeIndex >= readIndex)
            result += mAllocatedSize;

        return result;
    }

    size_t getAvailableRead(size_t writeIndex, size_t readIndex) const
    {
        if (writeIndex >= readIndex)
            return writeIndex - readIndex;

        return writeIndex + mAllocatedSize - readIndex;
    }

    T * mData;
    size_t mAllocatedSize;
    std::atomic<size_t> mWriteIndex, mReadIndex;
};


template <typename Data>
class ConcurrentQueue
{
    std::queue<Data> the_queue;
    mutable std::mutex the_mutex;
    std::condition_variable the_condition_variable;

public:
    void push(Data const & data)
    {
        {
            std::lock_guard<std::mutex> lock(the_mutex);
            the_queue.push(data);
        }
        the_condition_variable.notify_one();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_queue.empty();
    }

    bool try_pop(Data & popped_value)
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        if (the_queue.empty())
        {
            return false;
        }

        popped_value = the_queue.front();
        the_queue.pop();
        return true;
    }

    void wait_and_pop(Data & popped_value)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        while (the_queue.empty())
        {
            the_condition_variable.wait(lock);
        }

        popped_value = the_queue.front();
        the_queue.pop();
    }
};
}
