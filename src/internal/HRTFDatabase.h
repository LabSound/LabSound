// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef HRTFDatabase_h
#define HRTFDatabase_h

#include "LabSound/extended/Util.h"
#include "internal/FFTFrame.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace lab
{

class AudioChannel;

// Hard-coded to the IRCAM HRTF Database
struct HRTFDatabaseInfo
{
    std::string subjectName;
    std::string searchPath;
    float sampleRate;

    int minElevation = -45;
    int maxElevation = 90;
    int rawElevationAngleSpacing = 15;
    bool files_found_and_loaded = false;
 
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

// HRTF stands for Head-Related Transfer Function.
// HRTFKernel is a frequency-domain representation of an impulse-response used as part of the spatialized panning system.
// For a given azimuth / elevation angle there will be one HRTFKernel for the left ear transfer function, and one for the right ear.
// The leading delay (average group delay) for each impulse response is extracted:
//      m_fftFrame is the frequency-domain representation of the impulse response with the delay removed
//      m_frameDelay is the leading delay of the original impulse response.
class HRTFKernel
{
public:
    // Note: this is destructive on the passed in AudioChannel.
    // The length of channel must be a power of two.
    HRTFKernel(AudioChannel *, int fftSize, float sampleRate);

    HRTFKernel(std::unique_ptr<FFTFrame> fftFrame, float frameDelay, float sampleRate)
        : m_fftFrame(std::move(fftFrame))
        , m_frameDelay(frameDelay)
        , m_sampleRate(sampleRate)
    {
        //m_fftFrame->print();
    }

    FFTFrame * fftFrame() { return m_fftFrame.get(); }

    int fftSize() const { return m_fftFrame->fftSize(); }
    float frameDelay() const { return m_frameDelay; }

    // Converts back into impulse-response form.
    std::unique_ptr<AudioChannel> createImpulseResponse();

    float sampleRate() const { return m_sampleRate; }

private:
    // Note: this is destructive on the passed in AudioChannel.
    std::unique_ptr<FFTFrame> m_fftFrame;
    float m_frameDelay;
    float m_sampleRate;
};

typedef std::vector<std::shared_ptr<HRTFKernel>> HRTFKernelList;

// Given two HRTFKernels, and an interpolation factor x: 0 -> 1, returns an interpolated HRTFKernel.
std::unique_ptr<HRTFKernel> MakeInterpolatedKernel(HRTFKernel * kernel1, HRTFKernel * kernel2, float x);

// HRTFElevation contains all of the HRTFKernels (one left ear and one right ear per azimuth angle) for a particular elevation.
class HRTFElevation
{

public:
    // Loads and returns an HRTFElevation with the given HRTF database subject name and elevation from resources.
    // Normally, there will only be a single HRTF database set, but this API supports the possibility of multiple ones with different names.
    // Interpolated azimuths will be generated based on InterpolationFactor.
    // Valid values for elevation are -45 -> +90 in 15 degree increments.
    static std::unique_ptr<HRTFElevation> createForSubject(HRTFDatabaseInfo * info, int elevation);

    // Given two HRTFElevations, and an interpolation factor x: 0 -> 1, returns an interpolated HRTFElevation.
    static std::unique_ptr<HRTFElevation> createByInterpolatingSlices(HRTFDatabaseInfo * info, HRTFElevation * hrtfElevation1, HRTFElevation * hrtfElevation2, float x);

    // Returns the list of left or right ear HRTFKernels for all the azimuths going from 0 to 360 degrees.
    HRTFKernelList * kernelListL() { return m_kernelListL.get(); }
    HRTFKernelList * kernelListR() { return m_kernelListR.get(); }

    double elevationAngle() const { return m_elevationAngle; }
    unsigned numberOfAzimuths() const { return NumberOfTotalAzimuths; }

    // Returns the left and right kernels for the given azimuth index.
    // The interpolated delays based on azimuthBlend: 0 -> 1 are returned in frameDelayL and frameDelayR.
    void getKernelsFromAzimuth(double azimuthBlend, unsigned azimuthIndex, HRTFKernel *& kernelL, HRTFKernel *& kernelR, double & frameDelayL, double & frameDelayR);

    // Spacing, in degrees, between every azimuth loaded from resource.
    static const unsigned AzimuthSpacing;

    // Number of azimuths loaded from resource.
    static const unsigned NumberOfRawAzimuths;

    // Interpolates by this factor to get the total number of azimuths from every azimuth loaded from resource.
    static const unsigned InterpolationFactor;

    // Total number of azimuths after interpolation.
    static const unsigned NumberOfTotalAzimuths;

    // Given a specific azimuth and elevation angle, returns the left and right HRTFKernel.
    // Valid values for azimuth are 0 -> 345 in 15 degree increments.
    // Valid values for elevation are -45 -> +90 in 15 degree increments.
    // Returns true on success.
    static bool calculateKernelsForAzimuthElevation(HRTFDatabaseInfo * info, int azimuth, int elevation, std::shared_ptr<HRTFKernel> & kernelL, std::shared_ptr<HRTFKernel> & kernelR);

    // Given a specific azimuth and elevation angle, returns the left and right HRTFKernel in kernelL and kernelR.
    // This method averages the measured response using symmetry of azimuth (for example by averaging the -30.0 and +30.0 azimuth responses).
    // Returns true on success.
    static bool calculateSymmetricKernelsForAzimuthElevation(HRTFDatabaseInfo * info, int azimuth, int elevation, std::shared_ptr<HRTFKernel> & kernelL, std::shared_ptr<HRTFKernel> & kernelR);

private:
    HRTFElevation(HRTFDatabaseInfo * info, std::unique_ptr<HRTFKernelList> kernelListL, std::unique_ptr<HRTFKernelList> kernelListR, int elevation)
        : m_kernelListL(std::move(kernelListL))
        , m_kernelListR(std::move(kernelListR))
        , m_elevationAngle(elevation)
    {
    }

    std::unique_ptr<HRTFKernelList> m_kernelListL;
    std::unique_ptr<HRTFKernelList> m_kernelListR;

    double m_elevationAngle;
};

class HRTFDatabase
{

    NO_MOVE(HRTFDatabase);

public:
    HRTFDatabase(float sampleRate, const std::string & searchPath);

    // getKernelsFromAzimuthElevation() returns a left and right ear kernel, and an interpolated left and right frame delay for the given azimuth and elevation.
    // azimuthBlend must be in the range 0 -> 1.
    // Valid values for azimuthIndex are 0 -> HRTFElevation::NumberOfTotalAzimuths - 1 (corresponding to angles of 0 -> 360).
    // Valid values for elevationAngle are MinElevation -> MaxElevation.
    void getKernelsFromAzimuthElevation(double azimuthBlend, unsigned azimuthIndex, double elevationAngle, HRTFKernel *& kernelL, HRTFKernel *& kernelR, double & frameDelayL, double & frameDelayR);

    // Returns the number of different azimuth angles.
    static unsigned numberOfAzimuths() { return HRTFElevation::NumberOfTotalAzimuths; }
    int numberOfElevations() const { return (int) m_elevations.size(); }
    bool files_found_and_loaded() { return info->files_found_and_loaded; }

private:
    std::vector<std::unique_ptr<HRTFElevation>> m_elevations;

    std::unique_ptr<HRTFDatabaseInfo> info;
};

// HRTFDatabaseLoader will asynchronously load the default HRTFDatabase in a new thread.

class HRTFDatabaseLoader
{

public:
    // Both constructor and destructor must be called from the main thread.
    // It's expected that the singletons will be accessed instead.
    // @CBB the guts of the loader should be a private singleton, so that the loader can be constructed without a factory
    explicit HRTFDatabaseLoader(float sampleRate, const std::string & searchPath);

    // Lazily creates the singleton HRTFDatabaseLoader (if not already created) and starts loading asynchronously (when created the first time).
    // Returns the singleton HRTFDatabaseLoader.
    // Must be called from the main thread.
    static HRTFDatabaseLoader* MakeHRTFLoaderSingleton(float sampleRate, const std::string & searchPath);

    // Returns the singleton HRTFDatabaseLoader.
    static HRTFDatabaseLoader* loader() { return s_loader; }

    // Both constructor and destructor must be called from the main thread.
    ~HRTFDatabaseLoader();

    // Returns true once the default database has been completely loaded.
    bool isLoaded() const;

    // waitForLoaderThreadCompletion() may be called more than once and is thread-safe.
    void waitForLoaderThreadCompletion();

    HRTFDatabase * database() { return m_hrtfDatabase.get(); }

    float databaseSampleRate() const { return m_databaseSampleRate; }

    // Called in asynchronous loading thread.
    void load();

    // defaultHRTFDatabase() gives access to the loaded database.
    // This can be called from any thread, but it is the callers responsibilty to call this while the context (and thus HRTFDatabaseLoader)
    // is still alive.  Otherwise this will return 0.
    static HRTFDatabase * defaultHRTFDatabase();

        // If it hasn't already been loaded, creates a new thread and initiates asynchronous loading of the default database.
    // This must be called from the main thread.
    void loadAsynchronously();

private:
    static void databaseLoaderEntry(HRTFDatabaseLoader * threadData);


    static HRTFDatabaseLoader* s_loader;  // convenience pointer, owned elsewhere
    std::unique_ptr<HRTFDatabase> m_hrtfDatabase;

    // Holding a m_threadLock is required when accessing m_databaseLoaderThread.
    std::mutex m_threadLock;
    std::thread m_databaseLoaderThread;
    std::condition_variable m_loadingCondition;
    bool m_loading;

    float m_databaseSampleRate;

    std::string searchPath;
};

}  // namespace lab

#endif  // HRTFDatabase_h
