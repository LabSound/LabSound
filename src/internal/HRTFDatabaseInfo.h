// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef HRTFDatabaseInfo_h
#define HRTFDatabaseInfo_h

#include "LabSound/extended/Util.h"
#include <string>

namespace lab
{

// Hard-coded to the IRCAM HRTF Database
struct HRTFDatabaseInfo
{
    std::string subjectName;
    std::string searchPath;
    float sampleRate;
    
    int minElevation = -45;
    int maxElevation = 90;
    int rawElevationAngleSpacing = 15;
    
    // Number of elevations loaded from resource
    int numberOfRawElevations = 10;  // -45 -> +90 (each 15 degrees)
    
    // Interpolates by this factor to get the total number of elevations from every elevation loaded from resource
    int interpolationFactor = 1;
    
    // Total number of elevations after interpolation.
    int numTotalElevations;
    
    HRTFDatabaseInfo(const std::string & subjectName, const std::string & searchPath, float sampleRate)
    : subjectName(subjectName)
    , searchPath(searchPath)
    , sampleRate(sampleRate)
    {
        numTotalElevations = numberOfRawElevations * interpolationFactor;
    }
    
    // Returns the index for the correct HRTFElevation given the elevation angle.
    int indexFromElevationAngle(double elevationAngle)
    {
        elevationAngle = Max((double) minElevation, elevationAngle);
        elevationAngle = Min((double) maxElevation, elevationAngle);
        return (int) (interpolationFactor * (elevationAngle - minElevation) / rawElevationAngleSpacing);
    }
};

}  // namespace lab

#endif  // HRTFDatabase_h
