// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/AudioUtilities.h"

#include <WTF/MathExtras.h>

namespace lab { namespace AudioUtilities {

float decibelsToLinear(float decibels)
{
    return powf(10, 0.05f * decibels);
}

float linearToDecibels(float linear)
{
    // It's not possible to calculate decibels for a zero linear value since it would be -Inf.
    // -1000.0 dB represents a very tiny linear value in case we ever reach this case.
    if (!linear)
        return -1000;
        
    return 20 * log10f(linear);
}

double discreteTimeConstantForSampleRate(double timeConstant, double sampleRate)
{
    return 1 - exp(-1 / (sampleRate * timeConstant));
}

size_t timeToSampleFrame(double time, double sampleRate)
{
    ASSERT(time >= 0);
    return static_cast<size_t>(round(time * sampleRate));
}
    
} } // end AudioUtilites & lab

