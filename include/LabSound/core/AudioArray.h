
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioArray_h
#define AudioArray_h

#include <stdint.h>
#include <stdlib.h>
#include <string.h> // for memset

namespace lab
{

template <typename T>
class AudioArray
{
    T * _allocation = nullptr;
    T * _data = nullptr;
    int _size = 0;
    T _safety;

public:
    explicit AudioArray()
    : _allocation(nullptr)
    , _data(nullptr)
    , _size(0)
    , _safety(0) {}
    
    explicit AudioArray(int n)
    : _allocation(nullptr)
    , _data(nullptr)
    , _size(0)
    , _safety(0)
    {
        allocate(n);
    }

    ~AudioArray()
    {
        free(_allocation);
    }

    // allocation will realloc if necessary.
    // the buffer will be zeroed whether reallocated or not
    //
    void allocate(int n)
    {
        const uintptr_t alignment = 0x10;
        const uintptr_t mask = ~0xf;
        size_t initialSize = sizeof(T) * n + alignment;
        if (_size != n) {
            free(_allocation);

            if (n > 0) {
                _allocation = static_cast<T*>(calloc(initialSize, 1));
                _data = (T*)((((intptr_t)_allocation) + (alignment-1)) & mask);
                _size = n;
            }
            else {
                _allocation = nullptr;
                _data = nullptr;
                _size = 0;
            }
        }
    }

    T * data() { return _data; }
    const T * data() const { return _data; }

    // size in samples, not bytes
    int size() const { return _size; }

    T & operator[](size_t i) {
        if (_data)
            return _data[i];
        return _safety;
    }

    void zero()
    {
        if (_data)
            memset(_data, 0, sizeof(T) * size());
    }

    void zeroRange(unsigned start, unsigned end)
    {
        if (_data == nullptr || _size <= 0)
            return;
        
        bool isSafe = (start <= end) && (end <= (unsigned) size());
        if (!isSafe)
            return;

        memset(_data + start, 0, sizeof(T) * (end - start));
    }

    void copyToRange(const T * sourceData, unsigned start, unsigned end)
    {
        if (_data == nullptr || _size <= 0)
            return;

        bool isSafe = (start <= end) && (end <= (unsigned) size());
        if (!isSafe)
            return;

        memcpy(_data + start, sourceData, sizeof(T) * (end - start));
    }

};

typedef AudioArray<float> AudioFloatArray;

}  // lab

#endif  // AudioArray_h
