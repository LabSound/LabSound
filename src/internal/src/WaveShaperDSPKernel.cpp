// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/Macros.h"
#include "internal/WaveShaperDSPKernel.h"
#include "internal/WaveShaperProcessor.h"
#include "internal/Assertions.h"

#include <algorithm>

using namespace std;

namespace lab {

void WaveShaperDSPKernel::process(ContextRenderLock &, const float* source, float* destination, size_t framesToProcess)
{
    ASSERT(source && destination && waveShaperProcessor());

    // Curve object locks the curve during processing
    std::unique_ptr<WaveShaperProcessor::Curve> c = std::move(waveShaperProcessor()->curve());

    if (c->curve.size() == 0) 
    {
        // Act as "straight wire" pass-through if no curve is set.
        memcpy(destination, source, sizeof(float) * framesToProcess);
        return;
    }

    float const * curveData = c->curve.data();
    size_t curveLength = c->curve.size();

    ASSERT(curveData);

    if (!curveData || !curveLength) 
    {
        memcpy(destination, source, sizeof(float) * framesToProcess);
        return;
    }

    // Apply waveshaping curve.
    for (unsigned i = 0; i < framesToProcess; ++i) 
    {
        const float input = source[i];

        // Calculate an index based on input -1 -> +1 with 0 being at the center of the curve data.
        size_t index = static_cast<size_t>((curveLength * (input + 1)) / 2);

        // Clip index to the input range of the curve.
        // This takes care of input outside of nominal range -1 -> +1
        index = max(index, size_t(0));
        index = min(index, curveLength - 1);
        destination[i] = curveData[index];
    }
}

} // namespace lab
