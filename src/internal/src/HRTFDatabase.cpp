// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/HRTFDatabase.h"
#include "internal/HRTFElevation.h"

using namespace std;

namespace lab {

const int HRTFDatabase::MinElevation = -45;
const int HRTFDatabase::MaxElevation = 90;
const unsigned HRTFDatabase::RawElevationAngleSpacing = 15;
const unsigned HRTFDatabase::NumberOfRawElevations = 10; // -45 -> +90 (each 15 degrees)
const unsigned HRTFDatabase::InterpolationFactor = 1;
const unsigned HRTFDatabase::NumberOfTotalElevations = NumberOfRawElevations * InterpolationFactor;

HRTFDatabase::HRTFDatabase(float sampleRate) : m_elevations(NumberOfTotalElevations) , m_sampleRate(sampleRate)
{
    unsigned elevationIndex = 0;
    for (int elevation = MinElevation; elevation <= MaxElevation; elevation += RawElevationAngleSpacing) {
        std::unique_ptr<HRTFElevation> hrtfElevation = HRTFElevation::createForSubject("Composite", elevation, sampleRate);
        // @Lab removed ASSERT(hrtfElevation.get());
        if (!hrtfElevation.get())
            return;
        
        m_elevations[elevationIndex] = std::move(hrtfElevation);
        elevationIndex += InterpolationFactor;
    }

    // Now, go back and interpolate elevations.
    if (InterpolationFactor > 1) {
        for (unsigned i = 0; i < NumberOfTotalElevations; i += InterpolationFactor) {
            unsigned j = (i + InterpolationFactor);
            if (j >= NumberOfTotalElevations)
                j = i; // for last elevation interpolate with itself

            // Create the interpolated convolution kernels and delays.
            for (unsigned jj = 1; jj < InterpolationFactor; ++jj) {
                float x = static_cast<float>(jj) / static_cast<float>(InterpolationFactor);
                m_elevations[i + jj] = HRTFElevation::createByInterpolatingSlices(m_elevations[i].get(), m_elevations[j].get(), x, sampleRate);
                ASSERT(m_elevations[i + jj].get());
            }
        }
    }
}

void HRTFDatabase::getKernelsFromAzimuthElevation(double azimuthBlend, unsigned azimuthIndex, double elevationAngle, HRTFKernel* &kernelL, HRTFKernel* &kernelR,
                                                  double& frameDelayL, double& frameDelayR)
{
    unsigned elevationIndex = indexFromElevationAngle(elevationAngle);
    ASSERT_WITH_SECURITY_IMPLICATION(elevationIndex < m_elevations.size() && m_elevations.size() > 0);
    
    if (!m_elevations.size()) {
        kernelL = 0;
        kernelR = 0;
        return;
    }
    
    if (elevationIndex > m_elevations.size() - 1)
        elevationIndex = m_elevations.size() - 1;    
    
    HRTFElevation* hrtfElevation = m_elevations[elevationIndex].get();
    /// @LAB removed ASSERT(hrtfElevation);
    if (!hrtfElevation) {
        kernelL = 0;
        kernelR = 0;
        return;
    }
    
    hrtfElevation->getKernelsFromAzimuth(azimuthBlend, azimuthIndex, kernelL, kernelR, frameDelayL, frameDelayR);
}                                                     

unsigned HRTFDatabase::indexFromElevationAngle(double elevationAngle)
{
    // Clamp to allowed range.
    elevationAngle = std::max(static_cast<double>(MinElevation), elevationAngle);
    elevationAngle = std::min(static_cast<double>(MaxElevation), elevationAngle);

    unsigned elevationIndex = static_cast<int>(InterpolationFactor * (elevationAngle - MinElevation) / RawElevationAngleSpacing);    
    return elevationIndex;
}

} // namespace lab
