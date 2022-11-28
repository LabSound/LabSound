// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/HRTFPanner.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/Macros.h"
#include "internal/Assertions.h"
#include "internal/FFTConvolver.h"
#include "internal/HRTFDatabase.h"
#include "internal/HRTFDatabaseLoader.h"
#include "LabSound/extended/Util.h"

#include <algorithm>

using namespace std;

namespace lab
{

// @fixme - will change for new HRTF databases
// The value of 2 milliseconds is larger than the largest delay which exists in any
// HRTFKernel from the default HRTFDatabase (0.0136 seconds).
// We ASSERT the delay values used in process() with this value.
const double MaxDelayTimeSeconds = 0.002;
const int UninitializedAzimuth = -1;
const uint32_t RenderingQuantum = AudioNode::ProcessingSizeInFrames;

HRTFPanner::HRTFPanner(uint32_t sampleRate, uint32_t fftSize)
    : Panner(sampleRate, PanningModel::HRTF)
    , m_crossfadeSelection(CrossfadeSelection1)
    , m_azimuthIndex1(UninitializedAzimuth)
    , m_elevation1(0)
    , m_azimuthIndex2(UninitializedAzimuth)
    , m_elevation2(0)
    , m_crossfadeX(0)
    , m_crossfadeIncr(0)
    , m_convolverL1(fftSize)
    , m_convolverR1(fftSize)
    , m_convolverL2(fftSize)
    , m_convolverR2(fftSize)
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

    uint32_t s = RoundNextPow2(sampleLength);
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

int HRTFPanner::calculateDesiredAzimuthIndexAndBlend(HRTFDatabase * database ,
                                                     double azimuth, double & azimuthBlend)
{
    // Convert the azimuth angle from the range -180 -> +180 into the range 0 -> 360.
    // The azimuth index may then be calculated from this positive value.
    if (azimuth < 0)
        azimuth += 360.0;

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
         const AudioBus& inputBus, AudioBus& outputBus,
         int busOffset,
         int framesToProcess)
{
    int numInputChannels = inputBus.numberOfChannels();

    bool isInputGood = numInputChannels >= Channels::Mono &&
                        numInputChannels <= Channels::Stereo &&
                        (framesToProcess + busOffset) <= inputBus.length();
    ASSERT(isInputGood);

    bool isOutputGood = outputBus.numberOfChannels() == Channels::Stereo &&
                            (framesToProcess + busOffset) <= outputBus.length();
    ASSERT(isOutputGood);

    if (!isInputGood || !isOutputGood)
    {
        outputBus.zero();
        return;
    }

    // This code only runs as long as the context is alive and after database has been loaded.
    HRTFDatabase * database = r.context()->hrtfDatabaseLoader()->database();
    if (!database)
    {
        outputBus.zero();
        return;
    }

    // IRCAM HRTF azimuths values from the loaded database is reversed
    // from the panner's notion of azimuth.
    double azimuth = -desiredAzimuth;

    bool isAzimuthGood = azimuth >= -180.0 && azimuth <= 180.0;
    ASSERT(isAzimuthGood);
    if (!isAzimuthGood)
    {
        outputBus.zero();
        return;
    }

    // Normally, mono sources are panned. However, if input is stereo,
    // implement stereo panning with left source
    // processed by left HRTF, and right source by right HRTF.
    const AudioChannel * inputChannelL = inputBus.channelByType(Channel::Left);
    const AudioChannel * inputChannelR = numInputChannels > Channels::Mono ?
                                            inputBus.channelByType(Channel::Right) : 0;

    // Get source and destination pointers.
    const float * sourceL = inputChannelL->data();
    const float * sourceR = numInputChannels > Channels::Mono ? inputChannelR->data() : sourceL;

    float * destinationL = outputBus.channelByType(Channel::Left)->mutableData();
    float * destinationR = outputBus.channelByType(Channel::Right)->mutableData();
    
    /// @TODO need to offset source and destination by offset
    /// however that will violate the power of two restriction below,
    /// so something needs to be changed to allow for partial buffers

    double azimuthBlend;
    int desiredAzimuthIndex = calculateDesiredAzimuthIndexAndBlend(
                                    database, azimuth, azimuthBlend);

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

    // This algorithm currently requires that we process in power-of-two size chunks
    // at least RenderingQuantum in size
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

        database->getKernelsFromAzimuthElevation(
                        azimuthBlend, m_azimuthIndex1, m_elevation1,
                        kernelL1, kernelR1, frameDelayL1, frameDelayR1);
        database->getKernelsFromAzimuthElevation(
                        azimuthBlend, m_azimuthIndex2, m_elevation2,
                        kernelL2, kernelR2, frameDelayL2, frameDelayR2);

        bool areKernelsGood = kernelL1 && kernelR1 && kernelL2 && kernelR2;
        ASSERT(areKernelsGood);

        if (!areKernelsGood)
        {
            outputBus.zero();
            return;
        }

        ASSERT(frameDelayL1 / r.context()->sampleRate() < MaxDelayTimeSeconds &&
               frameDelayR1 / r.context()->sampleRate() < MaxDelayTimeSeconds);
        ASSERT(frameDelayL2 / r.context()->sampleRate() < MaxDelayTimeSeconds &&
               frameDelayR2 / r.context()->sampleRate() < MaxDelayTimeSeconds);

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
        // Avoid doing convolutions on both sets of convolvers if we're not currently cross-fading.
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
    // Because HRTFPanner is implemented with a DelayKernel and a FFTConvolver,
    // the tailTime of the HRTFPanner
    // is the sum of the tailTime of the DelayKernel and the tailTime of the FFTConvolver,
    // which is MaxDelayTimeSeconds and fftSize() / 2, respectively.
    return MaxDelayTimeSeconds + (fftSize() / 2) / static_cast<double>(r.context()->sampleRate());
}

double HRTFPanner::latencyTime(ContextRenderLock & r) const
{
    // The latency of a FFTConvolver is also fftSize() / 2, and is in addition to
    // its tailTime of the same value.
    return (fftSize() / 2) / static_cast<double>(r.context()->sampleRate());
}

}  // namespace lab
