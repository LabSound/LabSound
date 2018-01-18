// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

// @tofix - webkit change ef113b changes the logic of processing to test for non finite time values, change not reflected here.
// @tofix - webkit change e369924 adds backward playback, change not incorporated yet

#include "LabSound/core/SampledAudioNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/Macros.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioUtilities.h"
#include "internal/Assertions.h"

#include <algorithm>

using namespace std;

namespace lab {

const double DefaultGrainDuration = 0.020; // 20ms

// Arbitrary upper limit on playback rate.
// Higher than expected rates can be useful when playing back oversampled buffers
// to minimize linear interpolation aliasing.
const double MaxRate = 1024;

SampledAudioNode::SampledAudioNode() : AudioScheduledSourceNode(), m_grainDuration(DefaultGrainDuration)
{
    m_gain = make_shared<AudioParam>("gain", 1.0, 0.0, 1.0);
    m_playbackRate = make_shared<AudioParam>("playbackRate", 1.0, 0.0, MaxRate);

    m_params.push_back(m_gain);
    m_params.push_back(m_playbackRate);

    // Default to mono. A call to setBus() will set the number of output channels to that of the bus.
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

    initialize();
}

SampledAudioNode::~SampledAudioNode()
{
    uninitialize();
}

void SampledAudioNode::process(ContextRenderLock & r, size_t framesToProcess)
{
    AudioBus* outputBus = output(0)->bus(r);

    if (!getBus() || !isInitialized() || ! r.context())
    {
        outputBus->zero();
        return;
    }

    // After calling setBuffer() with a buffer having a different number of channels, there can in rare cases be a slight delay
    // before the output bus is updated to the new number of channels because of use of tryLocks() in the context's updating system.
    // In this case, if the the buffer has just been changed and we're not quite ready yet, then just output silence. 
    if (numberOfChannels(r) != getBus()->numberOfChannels())
    {
        outputBus->zero();
        return;
    }

    if (m_startRequested) 
    {
        // Do sanity checking of grain parameters versus buffer size.
        double bufferDuration = duration();

        double grainOffset = std::max(0.0, m_requestGrainOffset);
        m_grainOffset = std::min(bufferDuration, grainOffset);
        m_grainOffset = grainOffset;

        // Handle default/unspecified duration.
        double maxDuration = bufferDuration - grainOffset;
        double grainDuration = m_requestGrainDuration;
        if (!grainDuration)
            grainDuration = maxDuration;

        grainDuration = std::max(0.0, grainDuration);
        grainDuration = std::min(maxDuration, grainDuration);
        m_grainDuration = grainDuration;

        m_isGrain = true;
        m_startTime = m_requestWhen;

        // We call timeToSampleFrame here since at playbackRate == 1 we don't want to go through linear interpolation
        // at a sub-sample position since it will degrade the quality.
        // When aligned to the sample-frame the playback will be identical to the PCM data stored in the buffer.
        // Since playbackRate == 1 is very common, it's worth considering quality.
        m_virtualReadIndex = AudioUtilities::timeToSampleFrame(m_grainOffset, m_sourceBus->sampleRate());
        m_startRequested = false;
    }

    size_t quantumFrameOffset;
    size_t bufferFramesToProcess;

    updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, bufferFramesToProcess);

    if (!bufferFramesToProcess)
    {
        outputBus->zero();
        return;
    }

    // Render by reading directly from the buffer.
    if (!renderFromBuffer(r, outputBus, quantumFrameOffset, bufferFramesToProcess))
    {
        outputBus->zero();
        return;
    }

    // Apply the gain (in-place) to the output bus.
    float totalGain = gain()->value(r);
    outputBus->copyWithGainFrom(*outputBus, &m_lastGain, totalGain);
    outputBus->clearSilentFlag();
}

// Returns true if we're finished.
bool SampledAudioNode::renderSilenceAndFinishIfNotLooping(ContextRenderLock& r, AudioBus * bus, unsigned index, size_t framesToProcess)
{
    if (!loop()) 
    {
        // If we're not looping, then stop playing when we get to the end.

        if (framesToProcess > 0) 
        {
            // We're not looping and we've reached the end of the sample data, but we still need to provide more output,
            // so generate silence for the remaining.
            for (unsigned i = 0; i < numberOfChannels(r); ++i)
            {
                memset(bus->channel(i)->mutableData() + index, 0, sizeof(float) * framesToProcess);
            }
        }

        finish(r);
        return true;
    }
    return false;
}

bool SampledAudioNode::renderFromBuffer(ContextRenderLock & r, AudioBus * bus, unsigned destinationFrameOffset, size_t numberOfFrames)
{
    if (!r.context())
        return false;

    // Basic sanity checking
    ASSERT(bus);
    ASSERT(getBus());
    if (!bus || !getBus())
        return false;

    unsigned numChannels = numberOfChannels(r);
    unsigned busNumberOfChannels = bus->numberOfChannels();

    bool channelCountGood = numChannels && numChannels == busNumberOfChannels;
    ASSERT(channelCountGood);
    if (!channelCountGood)
        return false;

    // Sanity check destinationFrameOffset, numberOfFrames.
    size_t destinationLength = bus->length();

    bool isLengthGood = destinationLength <= 4096 && numberOfFrames <= 4096;
    ASSERT(isLengthGood);
    if (!isLengthGood)
        return false;

    bool isOffsetGood = destinationFrameOffset <= destinationLength && destinationFrameOffset + numberOfFrames <= destinationLength;
    ASSERT(isOffsetGood);
    if (!isOffsetGood)
        return false;

    // Offset the pointers to the correct offset frame.
    unsigned writeIndex = destinationFrameOffset;

    size_t bufferLength = getBus()->length();
    double bufferSampleRate = getBus()->sampleRate();

    // Avoid converting from time to sample-frames twice by computing
    // the grain end time first before computing the sample frame.
    unsigned endFrame = m_isGrain ? AudioUtilities::timeToSampleFrame(m_grainOffset + m_grainDuration, bufferSampleRate) : bufferLength;

    // This is a HACK to allow for HRTF tail-time - avoids glitch at end.
    // FIXME: implement tailTime for each AudioNode for a more general solution to this problem.
    // https://bugs.webkit.org/show_bug.cgi?id=77224
    if (m_isGrain)
        endFrame += 512;

    // Do some sanity checking.
    if (endFrame > bufferLength)
        endFrame = bufferLength;

    if (m_virtualReadIndex >= endFrame)
        m_virtualReadIndex = 0; // reset to start

    // If the .loop attribute is true, then values of m_loopStart == 0 && m_loopEnd == 0 implies
    // that we should use the entire buffer as the loop, otherwise use the loop values in m_loopStart and m_loopEnd.
    double virtualEndFrame = endFrame;
    double virtualDeltaFrames = endFrame;

    if (loop() && (m_loopStart || m_loopEnd) && m_loopStart >= 0 && m_loopEnd > 0 && m_loopStart < m_loopEnd) 
    {
        // Convert from seconds to sample-frames.
        double loopStartFrame = m_loopStart * getBus()->sampleRate();
        double loopEndFrame = m_loopEnd * getBus()->sampleRate();

        virtualEndFrame = std::min(loopEndFrame, virtualEndFrame);
        virtualDeltaFrames = virtualEndFrame - loopStartFrame;
    }


    double pitchRate = totalPitchRate(r);

    // Sanity check that our playback rate isn't larger than the loop size.
    if (fabs(pitchRate) >= virtualDeltaFrames)
        return false;

    // Get local copy.
    double virtualReadIndex = m_virtualReadIndex;

    // Render loop - reading from the source buffer to the destination using linear interpolation.
    int framesToProcess = numberOfFrames;

    // Optimize for the very common case of playing back with pitchRate == 1.
    // We can avoid the linear interpolation.
    if (pitchRate == 1 && virtualReadIndex == floor(virtualReadIndex) && virtualDeltaFrames == floor(virtualDeltaFrames) && virtualEndFrame == floor(virtualEndFrame)) 
    {
        unsigned readIndex = static_cast<unsigned>(virtualReadIndex);
        unsigned deltaFrames = static_cast<unsigned>(virtualDeltaFrames);
        endFrame = static_cast<unsigned>(virtualEndFrame);

        while (framesToProcess > 0) 
        {
            int framesToEnd = endFrame - readIndex;
            int framesThisTime = std::min(framesToProcess, framesToEnd);
            framesThisTime = std::max(0, framesThisTime);

            for (unsigned i = 0; i < numChannels; ++i)
            {
                memcpy(bus->channel(i)->mutableData() + writeIndex, m_sourceBus->channel(i)->data() + readIndex, sizeof(float) * framesThisTime);
            }

            writeIndex += framesThisTime;
            readIndex += framesThisTime;
            framesToProcess -= framesThisTime;

            // Wrap-around.
            if (readIndex >= endFrame) 
            {
                readIndex -= deltaFrames;
                if (renderSilenceAndFinishIfNotLooping(r, bus, writeIndex, framesToProcess))
                    break;
            }
        }
        virtualReadIndex = readIndex;
    } 
    else 
    {
        while (framesToProcess--) 
        {
            unsigned readIndex = static_cast<unsigned>(virtualReadIndex);
            double interpolationFactor = virtualReadIndex - readIndex;

            // For linear interpolation we need the next sample-frame too.
            unsigned readIndex2 = readIndex + 1;

            if (readIndex2 >= bufferLength) 
            {
                if (loop()) {
                    // Make sure to wrap around at the end of the buffer.
                    readIndex2 = static_cast<unsigned>(virtualReadIndex + 1 - virtualDeltaFrames);
                } else
                    readIndex2 = readIndex;
            }

            // Final sanity check on buffer access.
            // FIXME: as an optimization, try to get rid of this inner-loop check and put assertions and guards before the loop.
            if (readIndex >= bufferLength || readIndex2 >= bufferLength)
                break;

            // Linear interpolation.
            for (unsigned i = 0; i < numChannels; ++i) 
            {
                float * destination = bus->channel(i)->mutableData();
                const float * source = m_sourceBus->channel(i)->data();

                double sample1 = source[readIndex];
                double sample2 = source[readIndex2];
                double sample = (1.0 - interpolationFactor) * sample1 + interpolationFactor * sample2;

                destination[writeIndex] = static_cast<float>(sample);
            }
            writeIndex++;

            virtualReadIndex += pitchRate;

            // Wrap-around, retaining sub-sample position since virtualReadIndex is floating-point.
            if (virtualReadIndex >= virtualEndFrame) 
            {
                virtualReadIndex -= virtualDeltaFrames;
                if (renderSilenceAndFinishIfNotLooping(r, bus, writeIndex, framesToProcess))
                    break;
            }
        }
    }

    bus->clearSilentFlag();

    m_virtualReadIndex = virtualReadIndex;

    return true;
}


void SampledAudioNode::reset(ContextRenderLock& r)
{
    m_virtualReadIndex = 0;
    m_lastGain = gain()->value(r);
    AudioScheduledSourceNode::reset(r);
}

bool SampledAudioNode::setBus(ContextRenderLock & r, std::shared_ptr<AudioBus> buffer)
{
    ASSERT(r.context());

    if (buffer) 
    {
        // Do any necesssary re-configuration to the buffer's number of channels.
        unsigned numberOfChannels = buffer->numberOfChannels();

        if (numberOfChannels > AudioContext::maxNumberOfChannels)
            return false;

        output(0)->setNumberOfChannels(r, numberOfChannels);
    }

    m_virtualReadIndex = 0;
    m_sourceBus = buffer;
    return true;
}

unsigned SampledAudioNode::numberOfChannels(ContextRenderLock& r)
{
    return output(0)->numberOfChannels();
}

void SampledAudioNode::startGrain(double when, double grainOffset)
{
    // Duration of 0 has special value, meaning calculate based on the entire buffer's duration.
    startGrain(when, grainOffset, 0);
}

void SampledAudioNode::startGrain(double when, double grainOffset, double grainDuration)
{
    if (!getBus())
        return;

    m_requestWhen = when;
    m_requestGrainOffset = grainOffset;
    m_requestGrainDuration = grainDuration;

    m_playbackState = SCHEDULED_STATE;
    m_startRequested = true;
}

float SampledAudioNode::duration() const 
{ 
    return m_sourceBus->length() / m_sourceBus->sampleRate(); 
}

double SampledAudioNode::totalPitchRate(ContextRenderLock & r)
{
    double dopplerRate = 1.0;
    if (m_pannerNode)
        dopplerRate = m_pannerNode->dopplerRate(r);

    // Incorporate buffer's sample-rate versus AudioContext's sample-rate.
    // Normally it's not an issue because buffers are loaded at the AudioContext's sample-rate, but we can handle it in any case.
    double sampleRateFactor = 1.0;
    if (getBus())
        sampleRateFactor = getBus()->sampleRate() / r.context()->sampleRate();

    double basePitchRate = playbackRate()->value(r);

    double totalRate = dopplerRate * sampleRateFactor * basePitchRate;

    // Sanity check the total rate.  It's very important that the resampler not get any bad rate values.
    totalRate = std::max(0.0, totalRate);
    if (!totalRate)
        totalRate = 1; // zero rate is considered illegal
    totalRate = std::min(MaxRate, totalRate);

    bool isTotalRateValid = !std::isnan(totalRate) && !std::isinf(totalRate);
    ASSERT(isTotalRateValid);
    if (!isTotalRateValid)
        totalRate = 1.0;

    return totalRate;
}

bool SampledAudioNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished() || !m_sourceBus;
}

void SampledAudioNode::setPannerNode(PannerNode* pannerNode)
{
    m_pannerNode = pannerNode;
}

void SampledAudioNode::clearPannerNode()
{
    m_pannerNode = 0;
}

} // namespace lab

