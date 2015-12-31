// License: BSD 1 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

// FFTFrame stub implementation to avoid link errors during bringup

#include "internal/ConfigMacros.h"

#if !OS(DARWIN) && !USE(WEBAUDIO_KISSFFT)

#include "internal/FFTFrame.h"

namespace lab {

// Normal constructor: allocates for a given fftSize.
FFTFrame::FFTFrame(unsigned /*fftSize*/)
    : m_FFTSize(0)
    , m_log2FFTSize(0)
{
    ASSERT_NOT_REACHED();
}

// Creates a blank/empty frame (interpolate() must later be called).
FFTFrame::FFTFrame()
    : m_FFTSize(0)
    , m_log2FFTSize(0)
{
    ASSERT_NOT_REACHED();
}

// Copy constructor.
FFTFrame::FFTFrame(const FFTFrame& frame)
    : m_FFTSize(frame.m_FFTSize)
    , m_log2FFTSize(frame.m_log2FFTSize)
{
    ASSERT_NOT_REACHED();
}

FFTFrame::~FFTFrame()
{
    ASSERT_NOT_REACHED();
}

void FFTFrame::multiply(const FFTFrame& frame)
{
    ASSERT_NOT_REACHED();
}

void FFTFrame::doFFT(const float* data)
{
    ASSERT_NOT_REACHED();
}

void FFTFrame::doInverseFFT(float* data)
{
    ASSERT_NOT_REACHED();
}

void FFTFrame::initialize()
{
}

void FFTFrame::cleanup()
{
    ASSERT_NOT_REACHED();
}

float* FFTFrame::realData() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

float* FFTFrame::imagData() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

} // namespace lab

#endif // !OS(DARWIN)
