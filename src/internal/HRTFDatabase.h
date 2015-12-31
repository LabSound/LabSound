// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef HRTFDatabase_h
#define HRTFDatabase_h

#include "LabSound/extended/Util.h"
#include "internal/HRTFElevation.h"

#include <vector>

namespace lab {

class HRTFKernel;

class HRTFDatabase 
{
	NO_MOVE(HRTFDatabase);
public:

	HRTFDatabase(float sampleRate);

    // getKernelsFromAzimuthElevation() returns a left and right ear kernel, and an interpolated left and right frame delay for the given azimuth and elevation.
    // azimuthBlend must be in the range 0 -> 1.
    // Valid values for azimuthIndex are 0 -> HRTFElevation::NumberOfTotalAzimuths - 1 (corresponding to angles of 0 -> 360).
    // Valid values for elevationAngle are MinElevation -> MaxElevation.
    void getKernelsFromAzimuthElevation(double azimuthBlend, unsigned azimuthIndex, double elevationAngle, HRTFKernel* &kernelL, HRTFKernel* &kernelR, double& frameDelayL, double& frameDelayR);

    // Returns the number of different azimuth angles.
    static unsigned numberOfAzimuths() { return HRTFElevation::NumberOfTotalAzimuths; }

    float sampleRate() const { return m_sampleRate; }

    // Number of elevations loaded from resource.
    static const unsigned NumberOfRawElevations;

private:

    // Minimum and maximum elevation angles (inclusive) for a HRTFDatabase.
    static const int MinElevation;
    static const int MaxElevation;
    static const unsigned RawElevationAngleSpacing;

    // Interpolates by this factor to get the total number of elevations from every elevation loaded from resource.
    static const unsigned InterpolationFactor;
    
    // Total number of elevations after interpolation.
    static const unsigned NumberOfTotalElevations;

    // Returns the index for the correct HRTFElevation given the elevation angle.
    static unsigned indexFromElevationAngle(double);

    std::vector<std::unique_ptr<HRTFElevation> > m_elevations;
    float m_sampleRate;
};

} // namespace lab

#endif // HRTFDatabase_h
