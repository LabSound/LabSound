// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/HRTFPanner.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/Macros.h"
#include "LabSound/extended/AudioFileReader.h"
#include "internal/Assertions.h"
#include "internal/HRTFDatabase.h"
#include "internal/Biquad.h"
#include "internal/FFTConvolver.h"
#include "internal/FFTFrame.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <math.h>
#include <string>

#include <algorithm>

using namespace std;

#if defined(_MSC_VER)
// suppress warnings about snptrintf
#pragma warning(disable : 4996)
#endif


namespace lab
{



HRTFDatabase::HRTFDatabase(float sampleRate, const std::string & searchPath)
{
    info.reset(new HRTFDatabaseInfo("Composite", searchPath, sampleRate));

    m_elevations.resize(info->numTotalElevations);

    int elevationIndex = 0;
    for (int elevation = info->minElevation; elevation <= info->maxElevation; elevation += info->rawElevationAngleSpacing)
    {
        std::unique_ptr<HRTFElevation> hrtfElevation = HRTFElevation::createForSubject(info.get(), elevation);

        // @tofix - removed ASSERT(hrtfElevation.get());
        if (!hrtfElevation.get()) return;

        m_elevations[elevationIndex] = std::move(hrtfElevation);
        elevationIndex += info->interpolationFactor;
    }

    // Go back and interpolate elevations
    if (info->interpolationFactor > 1)
    {
        for (int i = 0; i < info->numTotalElevations; i += info->interpolationFactor)
        {
            int j = (i + info->interpolationFactor);

            if (j >= info->numTotalElevations)
            {
                j = i;  // for last elevation interpolate with itself
            }

            // Create the interpolated convolution kernels and delays.
            for (int jj = 1; jj < info->interpolationFactor; ++jj)
            {
                float x = static_cast<float>(jj) / static_cast<float>(info->interpolationFactor);
                m_elevations[i + jj] = HRTFElevation::createByInterpolatingSlices(info.get(), m_elevations[i].get(), m_elevations[j].get(), x);
                ASSERT(m_elevations[i + jj].get());
            }
        }
    }
}

void HRTFDatabase::getKernelsFromAzimuthElevation(double azimuthBlend,
                                                  unsigned azimuthIndex,
                                                  double elevationAngle,
                                                  HRTFKernel *& kernelL,
                                                  HRTFKernel *& kernelR,
                                                  double & frameDelayL,
                                                  double & frameDelayR)
{

    size_t elevationIndex = info->indexFromElevationAngle(elevationAngle);

    ASSERT(elevationIndex < m_elevations.size() && m_elevations.size() > 0);

    if (!m_elevations.size())
    {
        kernelL = 0;
        kernelR = 0;
        return;
    }

    if (elevationIndex > m_elevations.size() - 1)
    {
        elevationIndex = m_elevations.size() - 1;
    }

    HRTFElevation * hrtfElevation = m_elevations[elevationIndex].get();

    ASSERT(hrtfElevation);

    if (!hrtfElevation)
    {
        throw std::runtime_error("Error getting hrtfElevation");
    }

    hrtfElevation->getKernelsFromAzimuth(azimuthBlend, azimuthIndex, kernelL, kernelR, frameDelayL, frameDelayR);
}



// Singleton
HRTFDatabaseLoader* HRTFDatabaseLoader::s_loader;

HRTFDatabaseLoader* HRTFDatabaseLoader::MakeHRTFLoaderSingleton(float sampleRate, const std::string & searchPath)
{
    //if (!s_loader)
    //{
    //    s_loader = new HRTFDatabaseLoader(sampleRate, searchPath);
    //    s_loader->loadAsynchronously();
    //}
    if (!s_loader)
    {
        s_loader = new HRTFDatabaseLoader(sampleRate, searchPath);
        // s_loader->loadAsynchronously();
    }
    if (s_loader && (!s_loader->database() || !s_loader->database()->files_found_and_loaded()))
    {
        s_loader = new HRTFDatabaseLoader(sampleRate, searchPath);
    }
    // if (s_loader && s_loader->database() && (s_loader->searchPath != searchPath ) )
    //{
    //     s_loader = new HRTFDatabaseLoader(sampleRate, searchPath);
    // }

    return s_loader;
}

HRTFDatabaseLoader::HRTFDatabaseLoader(float sampleRate, const std::string & searchPath)
    : m_loading(false)
    , m_databaseSampleRate(sampleRate)
    , searchPath(searchPath)
{
    s_loader = this;
}

HRTFDatabaseLoader::~HRTFDatabaseLoader()
{
    waitForLoaderThreadCompletion();

    if (m_databaseLoaderThread.joinable()) m_databaseLoaderThread.join();

    m_hrtfDatabase.reset();

    ASSERT(this == s_loader);

    //s_loader = nullptr; //.reset(db) creates new loader before calling this destructor
}

// Asynchronously load the database in this thread.
void HRTFDatabaseLoader::databaseLoaderEntry(HRTFDatabaseLoader * threadData)
{
    std::lock_guard<std::mutex> locker(threadData->m_threadLock);
    HRTFDatabaseLoader * loader = reinterpret_cast<HRTFDatabaseLoader *>(threadData);
    ASSERT(loader);

    threadData->m_loading = true;
    loader->load();
    threadData->m_loadingCondition.notify_one();
}

void HRTFDatabaseLoader::load()
{
    m_hrtfDatabase.reset(new HRTFDatabase(m_databaseSampleRate, searchPath));

    if (!m_hrtfDatabase.get())
    {
        LOG_ERROR("HRTF database not loaded");
    }
}

void HRTFDatabaseLoader::loadAsynchronously()
{
    std::lock_guard<std::mutex> lock(m_threadLock);

    if (!m_hrtfDatabase.get() && !m_loading)
    {
        m_databaseLoaderThread = std::thread(databaseLoaderEntry, this);
    }
}

bool HRTFDatabaseLoader::isLoaded() const
{
    return (m_hrtfDatabase.get() != nullptr);
}

void HRTFDatabaseLoader::waitForLoaderThreadCompletion()
{
    std::unique_lock<std::mutex> locker(m_threadLock);
    while (!m_hrtfDatabase.get())
    {
        m_loadingCondition.wait(locker);
    }
}

HRTFDatabase * HRTFDatabaseLoader::defaultHRTFDatabase()
{
    if (!s_loader) return nullptr;
    return s_loader->database();
}


// The range of elevations for the IRCAM impulse responses varies depending on azimuth, but the minimum elevation appears to always be -45.
static int maxElevations[] = {
    // Azimuth
    90,  // 0
    45,  // 15
    60,  // 30
    45,  // 45
    75,  // 60
    45,  // 75
    60,  // 90
    45,  // 105
    75,  // 120
    45,  // 135
    60,  // 150
    45,  // 165
    75,  // 180
    45,  // 195
    60,  // 210
    45,  // 225
    75,  // 240
    45,  // 255
    60,  // 270
    45,  // 285
    75,  // 300
    45,  // 315
    60,  // 330
    45  // 345
};

const uint32_t HRTFElevation::AzimuthSpacing = 15;
const uint32_t HRTFElevation::NumberOfRawAzimuths = 360 / AzimuthSpacing;
const uint32_t HRTFElevation::InterpolationFactor = 8;
const uint32_t HRTFElevation::NumberOfTotalAzimuths = NumberOfRawAzimuths * InterpolationFactor;

// Takes advantage of the symmetry and creates a composite version of the two measured versions.  For example, we have both azimuth 30 and -30 degrees
// where the roles of left and right ears are reversed with respect to each other.
bool HRTFElevation::calculateSymmetricKernelsForAzimuthElevation(HRTFDatabaseInfo * info, int azimuth, int elevation, std::shared_ptr<HRTFKernel> & kernelL, std::shared_ptr<HRTFKernel> & kernelR)
{
    std::shared_ptr<HRTFKernel> kernelL1;
    std::shared_ptr<HRTFKernel> kernelR1;

    bool success = calculateKernelsForAzimuthElevation(info, azimuth, elevation, kernelL1, kernelR1);

    if (!success)
        return false;

    // And symmetric version
    int symmetricAzimuth = !azimuth ? 0 : 360 - azimuth;

    std::shared_ptr<HRTFKernel> kernelL2;
    std::shared_ptr<HRTFKernel> kernelR2;

    success = calculateKernelsForAzimuthElevation(info, symmetricAzimuth, elevation, kernelL2, kernelR2);

    if (!success) return false;

    // Notice L/R reversal in symmetric version.
    kernelL = MakeInterpolatedKernel(kernelL1.get(), kernelR2.get(), 0.5f);
    kernelR = MakeInterpolatedKernel(kernelR1.get(), kernelL2.get(), 0.5f);

    return true;
}

bool HRTFElevation::calculateKernelsForAzimuthElevation(
    HRTFDatabaseInfo * info, int azimuth, int elevation, 
    std::shared_ptr<HRTFKernel> & kernelL, std::shared_ptr<HRTFKernel> & kernelR)
{
    // Valid values for azimuth are 0 -> 345 in 15 degree increments.
    // Valid values for elevation are -45 -> +90 in 15 degree increments.

    bool isAzimuthGood = azimuth >= 0 && azimuth <= 345 && (azimuth / 15) * 15 == azimuth;
    ASSERT(isAzimuthGood);
    if (!isAzimuthGood) return false;

    bool isElevationGood = elevation >= -45 && elevation <= 90 && (elevation / 15) * 15 == elevation;
    ASSERT(isElevationGood);
    if (!isElevationGood) return false;

    // Construct the resource name from the subject name, azimuth, and elevation, for example:
    // "IRC_Composite_C_R0195_T015_P000"
    int positiveElevation = elevation < 0 ? elevation + 360 : elevation;

    char tempStr[16];

    // Located in $searchPath / [format] .wav
    // @tofix - this assumes we want to open this path and read via libnyquist fopen.
    // ... will need to change for Android. Maybe MakeBusFromInternalResource / along 
    // with a LoadInternalResources required by LabSound
    snprintf(tempStr, sizeof(tempStr), "%03d_P%03d", azimuth, positiveElevation);
    std::string resourceName = info->searchPath + "/" + "IRC_" + info->subjectName + "_C_R0195_T" + tempStr + ".wav";

    std::shared_ptr<AudioBus> impulseResponse = lab::MakeBusFromFile(resourceName.c_str(), false);

    if (!impulseResponse)
    {
        LOG_ERROR("impulse not found %s (bad path?)", resourceName.c_str());
        info->files_found_and_loaded = false;
        return false;
    }

    // The impulse files are 44.1k so we need to resample them if the graph is playing back at any other rate
    if (info->sampleRate != 44100.0f)
    {
        std::unique_ptr<AudioBus> resampledImpulseResponse = AudioBus::createBySampleRateConverting(impulseResponse.get(), false, info->sampleRate);
        impulseResponse = std::move(resampledImpulseResponse);
        // LOG_VERBOSE("converting %s impulse to %f hz", resourceName.c_str(), info->sampleRate);
    }

    // Check number of channels. For now these are fixed and known.
    bool isBusGood = (impulseResponse->numberOfChannels() == Channels::Stereo);

    ASSERT(isBusGood);
    if (!isBusGood)
    {
        info->files_found_and_loaded = false;
        return false;
    }

    AudioChannel * leftEarImpulseResponse = impulseResponse->channelByType(Channel::Left);
    AudioChannel * rightEarImpulseResponse = impulseResponse->channelByType(Channel::Right);

    // Note that depending on the fftSize returned by the panner, we may be truncating the impulse response we just loaded in.
    const int fftSize = HRTFPanner::fftSizeForSampleRate(info->sampleRate);
    kernelL = std::make_shared<HRTFKernel>(leftEarImpulseResponse, fftSize, info->sampleRate);
    kernelR = std::make_shared<HRTFKernel>(rightEarImpulseResponse, fftSize, info->sampleRate);
    info->files_found_and_loaded = true;

    return true;
}

std::unique_ptr<HRTFElevation> HRTFElevation::createForSubject(HRTFDatabaseInfo * info, int elevation)
{
    bool isElevationGood = elevation >= -45 && elevation <= 90 && (elevation / 15) * 15 == elevation;
    ASSERT(isElevationGood);

    if (!isElevationGood)
        return nullptr;

    std::unique_ptr<HRTFKernelList> kernelListL = std::unique_ptr<HRTFKernelList>(new HRTFKernelList(NumberOfTotalAzimuths));
    std::unique_ptr<HRTFKernelList> kernelListR = std::unique_ptr<HRTFKernelList>(new HRTFKernelList(NumberOfTotalAzimuths));

    // Load convolution kernels from HRTF files.
    int interpolatedIndex = 0;
    for (uint32_t rawIndex = 0; rawIndex < NumberOfRawAzimuths; ++rawIndex)
    {
        // Don't let elevation exceed maximum for this azimuth.
        int maxElevation = maxElevations[rawIndex];
        int actualElevation = min(elevation, maxElevation);

        bool success = calculateKernelsForAzimuthElevation(
            info,
            rawIndex * AzimuthSpacing, 
            actualElevation, kernelListL->at(interpolatedIndex), 
            kernelListR->at(interpolatedIndex));

        if (!success)
            return nullptr;

        interpolatedIndex += InterpolationFactor;
    }

    // Now go back and interpolate intermediate azimuth values.
    for (uint32_t i = 0; i < NumberOfTotalAzimuths; i += InterpolationFactor)
    {
        int j = (i + InterpolationFactor) % NumberOfTotalAzimuths;

        // Create the interpolated convolution kernels and delays.
        for (uint32_t jj = 1; jj < InterpolationFactor; ++jj)
        {
            float x = float(jj) / float(InterpolationFactor);  // interpolate from 0 -> 1

            (*kernelListL)[i + jj] = MakeInterpolatedKernel(kernelListL->at(i).get(), kernelListL->at(j).get(), x);
            (*kernelListR)[i + jj] = MakeInterpolatedKernel(kernelListR->at(i).get(), kernelListR->at(j).get(), x);
        }
    }

    return std::unique_ptr<HRTFElevation>(new HRTFElevation(info, std::move(kernelListL), std::move(kernelListR), elevation));
}

std::unique_ptr<HRTFElevation> HRTFElevation::createByInterpolatingSlices(HRTFDatabaseInfo * info, HRTFElevation * hrtfElevation1, HRTFElevation * hrtfElevation2, float x)
{
    ASSERT(hrtfElevation1 && hrtfElevation2);
    if (!hrtfElevation1 || !hrtfElevation2)
        return nullptr;

    ASSERT(x >= 0.0 && x < 1.0);

    std::unique_ptr<HRTFKernelList> kernelListL = std::unique_ptr<HRTFKernelList>(new HRTFKernelList(NumberOfTotalAzimuths));
    std::unique_ptr<HRTFKernelList> kernelListR = std::unique_ptr<HRTFKernelList>(new HRTFKernelList(NumberOfTotalAzimuths));

    HRTFKernelList * kernelListL1 = hrtfElevation1->kernelListL();
    HRTFKernelList * kernelListR1 = hrtfElevation1->kernelListR();
    HRTFKernelList * kernelListL2 = hrtfElevation2->kernelListL();
    HRTFKernelList * kernelListR2 = hrtfElevation2->kernelListR();

    // Interpolate kernels of corresponding azimuths of the two elevations.
    for (uint32_t i = 0; i < NumberOfTotalAzimuths; ++i)
    {
        (*kernelListL)[i] = MakeInterpolatedKernel(kernelListL1->at(i).get(), kernelListL2->at(i).get(), x);
        (*kernelListR)[i] = MakeInterpolatedKernel(kernelListR1->at(i).get(), kernelListR2->at(i).get(), x);
    }

    // Interpolate elevation angle.
    double angle = (1.0 - x) * hrtfElevation1->elevationAngle() + x * hrtfElevation2->elevationAngle();

    return std::unique_ptr<HRTFElevation>(new HRTFElevation(info, std::move(kernelListL), std::move(kernelListR), (int) angle));
}

void HRTFElevation::getKernelsFromAzimuth(double azimuthBlend, unsigned azimuthIndex, HRTFKernel *& kernelL, HRTFKernel *& kernelR, double & frameDelayL, double & frameDelayR)
{
    bool checkAzimuthBlend = azimuthBlend >= 0.0 && azimuthBlend < 1.0;
    ASSERT(checkAzimuthBlend);
    if (!checkAzimuthBlend)
    {
        azimuthBlend = 0.0;
    }

    size_t numKernels = m_kernelListL->size();

    bool isIndexGood = azimuthIndex < numKernels;
    ASSERT(isIndexGood);
    if (!isIndexGood)
    {
        kernelL = 0;
        kernelR = 0;
        return;
    }

    // Return the left and right kernels.
    kernelL = m_kernelListL->at(azimuthIndex).get();
    kernelR = m_kernelListR->at(azimuthIndex).get();

    frameDelayL = m_kernelListL->at(azimuthIndex)->frameDelay();
    frameDelayR = m_kernelListR->at(azimuthIndex)->frameDelay();

    int azimuthIndex2 = (azimuthIndex + 1) % numKernels;
    double frameDelay2L = m_kernelListL->at(azimuthIndex2)->frameDelay();
    double frameDelay2R = m_kernelListR->at(azimuthIndex2)->frameDelay();

    // Linearly interpolate delays.
    frameDelayL = (1.0 - azimuthBlend) * frameDelayL + azimuthBlend * frameDelay2L;
    frameDelayR = (1.0 - azimuthBlend) * frameDelayR + azimuthBlend * frameDelay2R;
}



// Takes the input AudioChannel as an input impulse response and calculates the average group delay.
// This represents the initial delay before the most energetic part of the impulse response.
// The sample-frame delay is removed from the impulseP impulse response, and this value  is returned.
// the length of the passed in AudioChannel must be a power of 2.
inline float ExtractAverageGroupDelay(AudioChannel * channel, int analysisFFTSize)
{
    ASSERT(channel);

    float * impulseP = channel->mutableData();

    bool isSizeGood = channel->length() >= analysisFFTSize;
    ASSERT(isSizeGood);
    if (!isSizeGood)
        return 0;

    // Check for power-of-2.
    ASSERT(uint64_t(1) << static_cast<uint64_t>(log2(analysisFFTSize)) == analysisFFTSize);

    FFTFrame estimationFrame(analysisFFTSize);
    estimationFrame.computeForwardFFT(impulseP);

    float frameDelay = static_cast<float>(estimationFrame.extractAverageGroupDelay());
    estimationFrame.computeInverseFFT(impulseP);

    return frameDelay;
}

HRTFKernel::HRTFKernel(AudioChannel * channel, int fftSize, float sampleRate)
    : m_frameDelay(0)
    , m_sampleRate(sampleRate)
{
    ASSERT(channel);

    // Determine the leading delay (average group delay) for the response.
    m_frameDelay = ExtractAverageGroupDelay(channel, fftSize / 2);

    float * impulseResponse = channel->mutableData();
    int responseLength = channel->length();

    // We need to truncate to fit into 1/2 the FFT size (with zero padding) in order to do proper convolution.
    int truncatedResponseLength = std::min(responseLength, fftSize / 2);  // truncate if necessary to max impulse response length allowed by FFT

    // Quick fade-out (apply window) at truncation point
    int numberOfFadeOutFrames = static_cast<unsigned>(sampleRate / 4410);  // 10 sample-frames @44.1KHz sample-rate
    ASSERT(numberOfFadeOutFrames < truncatedResponseLength);

    if (numberOfFadeOutFrames < truncatedResponseLength)
    {
        for (int i = truncatedResponseLength - numberOfFadeOutFrames; i < truncatedResponseLength; ++i)
        {
            float x = 1.0f - static_cast<float>(i - (truncatedResponseLength - numberOfFadeOutFrames)) / numberOfFadeOutFrames;
            impulseResponse[i] *= x;
        }
    }

    m_fftFrame = std::unique_ptr<FFTFrame>(new FFTFrame(fftSize));
    m_fftFrame->doPaddedFFT(impulseResponse, truncatedResponseLength);
}

std::unique_ptr<AudioChannel> HRTFKernel::createImpulseResponse()
{
    std::unique_ptr<AudioChannel> channel(new AudioChannel(fftSize()));
    FFTFrame fftFrame(*m_fftFrame);

    // Add leading delay back in.
    fftFrame.addConstantGroupDelay(m_frameDelay);
    fftFrame.computeInverseFFT(channel->mutableData());

    return channel;
}

// Interpolates two kernels with x: 0 -> 1 and returns the result.
std::unique_ptr<HRTFKernel> MakeInterpolatedKernel(HRTFKernel * kernel1, HRTFKernel * kernel2, float x)
{
    ASSERT(kernel1 && kernel2);
    if (!kernel1 || !kernel2)
        return 0;

    ASSERT(x >= 0.0 && x < 1.0);
    x = std::min(1.0f, std::max(0.0f, x));

    float sampleRate1 = kernel1->sampleRate();
    float sampleRate2 = kernel2->sampleRate();

    ASSERT(sampleRate1 == sampleRate2);
    if (sampleRate1 != sampleRate2)
        return 0;

    float frameDelay = (1 - x) * kernel1->frameDelay() + x * kernel2->frameDelay();
    //kernel1->fftFrame()->print();

    std::unique_ptr<FFTFrame> interpolatedFrame = FFTFrame::createInterpolatedFrame(*kernel1->fftFrame(), *kernel2->fftFrame(), x);
    return std::unique_ptr<HRTFKernel>(new HRTFKernel(std::move(interpolatedFrame), frameDelay, sampleRate1));
}


// @fixme - will change for new HRTF databases
// The value of 2 milliseconds is larger than the largest delay which exists in any HRTFKernel from the default HRTFDatabase (0.0136 seconds).
// We ASSERT the delay values used in process() with this value.
const double MaxDelayTimeSeconds = 0.002;
const int UninitializedAzimuth = -1;
const uint32_t RenderingQuantum = AudioNode::ProcessingSizeInFrames;

HRTFPanner::HRTFPanner(float sampleRate)
    : Panner(sampleRate, PanningModel::HRTF)
    , m_crossfadeSelection(CrossfadeSelection1)
    , m_azimuthIndex1(UninitializedAzimuth)
    , m_elevation1(0)
    , m_azimuthIndex2(UninitializedAzimuth)
    , m_elevation2(0)
    , m_crossfadeX(0)
    , m_crossfadeIncr(0)
    , m_convolverL1(fftSizeForSampleRate(sampleRate))
    , m_convolverR1(fftSizeForSampleRate(sampleRate))
    , m_convolverL2(fftSizeForSampleRate(sampleRate))
    , m_convolverR2(fftSizeForSampleRate(sampleRate))
    , m_delayLineL(MaxDelayTimeSeconds, sampleRate)
    , m_delayLineR(MaxDelayTimeSeconds, sampleRate)
    , m_tempL1(RenderingQuantum)
    , m_tempR1(RenderingQuantum)
    , m_tempL2(RenderingQuantum)
    , m_tempR2(RenderingQuantum)
{
}

HRTFPanner::~HRTFPanner()
{
}

uint32_t HRTFPanner::fftSizeForSampleRate(float sampleRate)
{
    // The HRTF impulse responses (loaded as audio resources) are 512 sample-frames @44.1KHz.
    // Currently, we truncate the impulse responses to half this size, but an FFT-size of twice impulse response size is needed (for convolution).
    // So for sample rates around 44.1KHz an FFT size of 512 is good. We double the FFT-size only for sample rates at least double this.
    ASSERT(sampleRate >= 44100 && sampleRate <= 96000.0);
    return (sampleRate < 88200.0) ? 512 : 1024;
}

//static 
uint32_t HRTFPanner::fftSizeForSampleLength(int sampleLength)
{
    // originally, this routine assumed 512 sample length impulse responses
    // truncated to 256 samples, and returned an fft size of 512 for those samples.
    // to match the reasoning, if the bus is not a power of 2 size, round down to the
    // previous power of 2 size.
    // return double the power of 2 size of the bus.
    // the original logic also doubled the fft size for sample rates >= 88200,
    // but that feels a bit nonsense, so it's not emulated here.

    int s = RoundNextPow2(sampleLength);
    if (s > sampleLength)
        s /= 2;

    return s * 2;
}

void HRTFPanner::reset()
{
    m_convolverL1.reset();
    m_convolverR1.reset();
    m_convolverL2.reset();
    m_convolverR2.reset();
    m_delayLineL.reset();
    m_delayLineR.reset();
}

int HRTFPanner::calculateDesiredAzimuthIndexAndBlend(double azimuth, double & azimuthBlend)
{
    // Convert the azimuth angle from the range -180 -> +180 into the range 0 -> 360.
    // The azimuth index may then be calculated from this positive value.
    if (azimuth < 0)
        azimuth += 360.0;

    HRTFDatabase * database = HRTFDatabaseLoader::defaultHRTFDatabase();
    ASSERT(database);

    int numberOfAzimuths = database->numberOfAzimuths();
    const double angleBetweenAzimuths = 360.0 / numberOfAzimuths;

    // Calculate the azimuth index and the blend (0 -> 1) for interpolation.
    double desiredAzimuthIndexFloat = azimuth / angleBetweenAzimuths;
    int desiredAzimuthIndex = static_cast<int>(desiredAzimuthIndexFloat);
    azimuthBlend = desiredAzimuthIndexFloat - static_cast<double>(desiredAzimuthIndex);

    // We don't immediately start using this azimuth index, but instead approach this index from the last index we rendered at.
    // This minimizes the clicks and graininess for moving sources which occur otherwise.
    desiredAzimuthIndex = max(0, desiredAzimuthIndex);
    desiredAzimuthIndex = min(numberOfAzimuths - 1, desiredAzimuthIndex);
    return desiredAzimuthIndex;
}

void HRTFPanner::pan(ContextRenderLock & r,
                double desiredAzimuth, double elevation,
                const AudioBus & inputBus, AudioBus & outputBus,
                int busOffset,
                int framesToProcess) 
{
    int numInputChannels = inputBus.numberOfChannels();

    bool isInputGood = numInputChannels >= Channels::Mono && numInputChannels <= Channels::Stereo;
    ASSERT(isInputGood);

    bool isOutputGood = outputBus.numberOfChannels() == Channels::Stereo && framesToProcess <= outputBus.length();
    ASSERT(isOutputGood);

    if (!isInputGood || !isOutputGood)
    {
        outputBus.zero();
        return;
    }

    // This code only runs as long as the context is alive and after database has been loaded.
    HRTFDatabase * database = HRTFDatabaseLoader::defaultHRTFDatabase();
    ASSERT(database);

    if (!database)
    {
        outputBus.zero();
        return;
    }

    // IRCAM HRTF azimuths values from the loaded database is reversed from the panner's notion of azimuth.
    double azimuth = -desiredAzimuth;

    bool isAzimuthGood = azimuth >= -180.0 && azimuth <= 180.0;
    ASSERT(isAzimuthGood);
    if (!isAzimuthGood)
    {
        outputBus.zero();
        return;
    }

    // Normally, we'll just be dealing with mono sources.
    // If we have a stereo input, implement stereo panning with left source processed by left HRTF, and right source by right HRTF.
    const AudioChannel * inputChannelL = inputBus.channelByType(Channel::Left);
    const AudioChannel * inputChannelR = numInputChannels > Channels::Mono ? inputBus.channelByType(Channel::Right) : 0;

    // Get source and destination pointers.
    const float * sourceL = inputChannelL->data();
    const float * sourceR = numInputChannels > Channels::Mono ? inputChannelR->data() : sourceL;

    float * destinationL = outputBus.channelByType(Channel::Left)->mutableData();
    float * destinationR = outputBus.channelByType(Channel::Right)->mutableData();

    double azimuthBlend;
    int desiredAzimuthIndex = calculateDesiredAzimuthIndexAndBlend(azimuth, azimuthBlend);

    // Initially snap azimuth and elevation values to first values encountered.
    if (m_azimuthIndex1 == UninitializedAzimuth)
    {
        m_azimuthIndex1 = desiredAzimuthIndex;
        m_elevation1 = elevation;
    }

    if (m_azimuthIndex2 == UninitializedAzimuth)
    {
        m_azimuthIndex2 = desiredAzimuthIndex;
        m_elevation2 = elevation;
    }

    // Cross-fade / transition over a period of around 45 milliseconds.
    // This is an empirical value tuned to be a reasonable trade-off between
    // smoothness and speed.
    const double fadeFrames = r.context()->sampleRate() <= 48000 ? 2048 : 4096;

    // Check for azimuth and elevation changes, initiating a cross-fade if needed.
    if (!m_crossfadeX && m_crossfadeSelection == CrossfadeSelection1)
    {
        if (desiredAzimuthIndex != m_azimuthIndex1 || elevation != m_elevation1)
        {
            // Cross-fade from 1 -> 2
            m_crossfadeIncr = static_cast<float>(1 / fadeFrames);
            m_azimuthIndex2 = desiredAzimuthIndex;
            m_elevation2 = elevation;
        }
    }

    if (m_crossfadeX == 1 && m_crossfadeSelection == CrossfadeSelection2)
    {
        if (desiredAzimuthIndex != m_azimuthIndex2 || elevation != m_elevation2)
        {
            // Cross-fade from 2 -> 1
            m_crossfadeIncr = static_cast<float>(-1 / fadeFrames);
            m_azimuthIndex1 = desiredAzimuthIndex;
            m_elevation1 = elevation;
        }
    }

    // This algorithm currently requires that we process in power-of-two size chunks at least RenderingQuantum.
    ASSERT(uint64_t(1) << static_cast<int>(log2(framesToProcess)) == framesToProcess);
    ASSERT(framesToProcess >= RenderingQuantum);

    const int framesPerSegment = RenderingQuantum;
    const int numberOfSegments = framesToProcess / framesPerSegment;

    for (int segment = 0; segment < numberOfSegments; ++segment)
    {
        // Get the HRTFKernels and interpolated delays.
        HRTFKernel * kernelL1;
        HRTFKernel * kernelR1;
        HRTFKernel * kernelL2;
        HRTFKernel * kernelR2;

        double frameDelayL1;
        double frameDelayR1;
        double frameDelayL2;
        double frameDelayR2;

        database->getKernelsFromAzimuthElevation(azimuthBlend, m_azimuthIndex1, m_elevation1, kernelL1, kernelR1, frameDelayL1, frameDelayR1);
        database->getKernelsFromAzimuthElevation(azimuthBlend, m_azimuthIndex2, m_elevation2, kernelL2, kernelR2, frameDelayL2, frameDelayR2);

        bool areKernelsGood = kernelL1 && kernelR1 && kernelL2 && kernelR2;
        ASSERT(areKernelsGood);

        if (!areKernelsGood)
        {
            outputBus.zero();
            return;
        }

        ASSERT(frameDelayL1 / r.context()->sampleRate() < MaxDelayTimeSeconds && frameDelayR1 / r.context()->sampleRate() < MaxDelayTimeSeconds);
        ASSERT(frameDelayL2 / r.context()->sampleRate() < MaxDelayTimeSeconds && frameDelayR2 / r.context()->sampleRate() < MaxDelayTimeSeconds);

        // Crossfade inter-aural delays based on transitions.
        double frameDelayL = (1 - m_crossfadeX) * frameDelayL1 + m_crossfadeX * frameDelayL2;
        double frameDelayR = (1 - m_crossfadeX) * frameDelayR1 + m_crossfadeX * frameDelayR2;

        // Calculate the source and destination pointers for the current segment.
        uint32_t offset = segment * framesPerSegment;
        const float * segmentSourceL = sourceL + offset;
        const float * segmentSourceR = sourceR + offset;
        float * segmentDestinationL = destinationL + offset;
        float * segmentDestinationR = destinationR + offset;

        // First run through delay lines for inter-aural time difference.
        m_delayLineL.setDelayFrames(frameDelayL);
        m_delayLineR.setDelayFrames(frameDelayR);
        m_delayLineL.process(r, segmentSourceL, segmentDestinationL, framesPerSegment);
        m_delayLineR.process(r, segmentSourceR, segmentDestinationR, framesPerSegment);

        bool needsCrossfading = m_crossfadeIncr;

        // Have the convolvers render directly to the final destination if we're not cross-fading.
        float * convolutionDestinationL1 = needsCrossfading ? m_tempL1.data() : segmentDestinationL;
        float * convolutionDestinationR1 = needsCrossfading ? m_tempR1.data() : segmentDestinationR;
        float * convolutionDestinationL2 = needsCrossfading ? m_tempL2.data() : segmentDestinationL;
        float * convolutionDestinationR2 = needsCrossfading ? m_tempR2.data() : segmentDestinationR;

        // Now do the convolutions.
        // Note that we avoid doing convolutions on both sets of convolvers if we're not currently cross-fading.
        if (m_crossfadeSelection == CrossfadeSelection1 || needsCrossfading)
        {
            m_convolverL1.process(kernelL1->fftFrame(), segmentDestinationL, convolutionDestinationL1, framesPerSegment);
            m_convolverR1.process(kernelR1->fftFrame(), segmentDestinationR, convolutionDestinationR1, framesPerSegment);
        }

        if (m_crossfadeSelection == CrossfadeSelection2 || needsCrossfading)
        {
            m_convolverL2.process(kernelL2->fftFrame(), segmentDestinationL, convolutionDestinationL2, framesPerSegment);
            m_convolverR2.process(kernelR2->fftFrame(), segmentDestinationR, convolutionDestinationR2, framesPerSegment);
        }

        if (needsCrossfading)
        {
            // Apply linear cross-fade.
            float x = m_crossfadeX;
            float incr = m_crossfadeIncr;

            for (uint32_t i = 0; i < framesPerSegment; ++i)
            {
                segmentDestinationL[i] = (1 - x) * convolutionDestinationL1[i] + x * convolutionDestinationL2[i];
                segmentDestinationR[i] = (1 - x) * convolutionDestinationR1[i] + x * convolutionDestinationR2[i];
                x += incr;
            }

            // Update cross-fade value from local.
            m_crossfadeX = x;

            if (m_crossfadeIncr > 0 && fabs(m_crossfadeX - 1) < m_crossfadeIncr)
            {
                // We've fully made the crossfade transition from 1 -> 2.
                m_crossfadeSelection = CrossfadeSelection2;
                m_crossfadeX = 1;
                m_crossfadeIncr = 0;
            }
            else if (m_crossfadeIncr < 0 && fabs(m_crossfadeX) < -m_crossfadeIncr)
            {
                // We've fully made the crossfade transition from 2 -> 1.
                m_crossfadeSelection = CrossfadeSelection1;
                m_crossfadeX = 0;
                m_crossfadeIncr = 0;
            }
        }
    }
}

double HRTFPanner::tailTime(ContextRenderLock & r) const
{
    // Because HRTFPanner is implemented with a DelayKernel and a FFTConvolver, the tailTime of the HRTFPanner
    // is the sum of the tailTime of the DelayKernel and the tailTime of the FFTConvolver, which is MaxDelayTimeSeconds
    // and fftSize() / 2, respectively.
    return MaxDelayTimeSeconds + (fftSize() / 2) / static_cast<double>(r.context()->sampleRate());
}

double HRTFPanner::latencyTime(ContextRenderLock & r) const
{
    // The latency of a FFTConvolver is also fftSize() / 2, and is in addition to its tailTime of the same value.
    return (fftSize() / 2) / static_cast<double>(r.context()->sampleRate());
}

}  // namespace lab
