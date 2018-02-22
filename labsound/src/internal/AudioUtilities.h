// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioUtilities_h
#define AudioUtilities_h

#include <sys/types.h>

namespace lab {

namespace AudioUtilities {

// Standard functions for converting to and from decibel values from linear.
float linearToDecibels(float);
float decibelsToLinear(float);

// timeConstant is the time it takes a first-order linear time-invariant system
// to reach the value 1 - 1/e (around 63.2%) given a step input response.
// discreteTimeConstantForSampleRate() will return the discrete time-constant for the specific sampleRate.
double discreteTimeConstantForSampleRate(double timeConstant, double sampleRate);

// Convert the time to a sample frame at the given sample rate.
size_t timeToSampleFrame(double time, double sampleRate);
} // AudioUtilites

} // lab

#endif // AudioUtilities_h
