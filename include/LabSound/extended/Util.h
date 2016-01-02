// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef LABSOUND_UTIL_H
#define LABSOUND_UTIL_H

#include <string>
#include <algorithm>

#define NO_COPY(C) C(const C &) = delete; C & operator = (const C &) = delete
#define NO_MOVE(C) NO_COPY(C); C(C &&) = delete; C & operator = (const C &&) = delete

namespace lab {
    
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
        int numberOfRawElevations = 10; // -45 -> +90 (each 15 degrees)
        
        // Interpolates by this factor to get the total number of elevations from every elevation loaded from resource
        int interpolationFactor = 1;
        
        // Total number of elevations after interpolation.
        int numTotalElevations;
        
        HRTFDatabaseInfo(const std::string & subjectName, const std::string & searchPath, float sampleRate):
            subjectName(subjectName),
            searchPath(searchPath),
            sampleRate(sampleRate)
        {
            numTotalElevations = numberOfRawElevations * interpolationFactor;
        }
        
        // Returns the index for the correct HRTFElevation given the elevation angle.
        int indexFromElevationAngle(double elevationAngle)
        {
            elevationAngle = std::max((double) minElevation, elevationAngle);
            elevationAngle = std::min((double) maxElevation, elevationAngle);
            return (int) (interpolationFactor * (elevationAngle - minElevation) / rawElevationAngleSpacing);
        }
    };
    
}

#endif
