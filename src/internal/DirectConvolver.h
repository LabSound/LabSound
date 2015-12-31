// License: BSD 3 Clause
// Copyright (C) 2012, Intel Corporation. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef DirectConvolver_h
#define DirectConvolver_h

#include "LabSound/core/AudioArray.h"

namespace lab 
{

class DirectConvolver 
{

public:

    DirectConvolver(size_t inputBlockSize);

    void process(AudioFloatArray* convolutionKernel, const float* sourceP, float* destP, size_t framesToProcess);

    void reset();

private:

    size_t m_inputBlockSize;
    AudioFloatArray m_buffer;
};

} // namespace lab

#endif // DirectConvolver_h
