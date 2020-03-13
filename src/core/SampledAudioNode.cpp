// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

// @tofix - webkit change ef113b changes the logic of processing to test for non finite time values, change not reflected here.
// @tofix - webkit change e369924 adds backward playback, change not incorporated yet

#include "LabSound/core/SampledAudioNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/VectorMath.h"
#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"

#include <algorithm>
#include <stack>

using namespace lab;

///////////////////////////
//   SampledAudioVoice   //
///////////////////////////

SampledAudioVoice::SampledAudioVoice(AudioContext & ac, float duration, std::shared_ptr<AudioParam> gain, std::shared_ptr<AudioParam> rate, 
        std::shared_ptr<AudioParam> detune, std::shared_ptr<AudioSetting> loop, std::shared_ptr<AudioSetting> loop_s, 
        std::shared_ptr<AudioSetting> loop_e,  std::shared_ptr<AudioSetting> src_bus) 
        : AudioNode(ac), m_duration(duration), m_gain(gain),  m_playbackRate(rate), m_detune(detune),
        m_isLooping(loop), m_loopStart(loop_s), m_loopEnd(loop_e), m_sourceBus(src_bus)
{
    initialize();
}

size_t SampledAudioVoice::numberOfChannels(ContextRenderLock & r)
{
    return m_inPlaceBus->numberOfChannels();
}


void SampledAudioVoice::updateSchedulingInfo(ContextRenderLock& r, size_t quantumFrameSize, AudioBus* outputBus, size_t& quantumFrameOffset, size_t& nonSilentFramesToProcess)
{
    if (!outputBus)  return;
    if (quantumFrameSize != AudioNode::ProcessingSizeInFrames) return;
    AudioContext* context = r.context();
    if (!context) return;

    // not atomic, but will largely prevent the times from being updated
    // by another thread during the update calculations.
    if (m_pendingEndTime > UNKNOWN_TIME)
    {
        m_endTime = m_pendingEndTime;
        m_pendingEndTime = UNKNOWN_TIME;
    }
    if (m_pendingStartTime > UNKNOWN_TIME)
    {
        m_startTime = m_pendingStartTime;
        m_pendingStartTime = UNKNOWN_TIME;
    }

    const float sampleRate = r.context()->sampleRate();

    // quantumStartFrame     : Start frame of the current time quantum.
    // quantumEndFrame       : End frame of the current time quantum.
    // startFrame            : Start frame for this source.
    // endFrame              : End frame for this source.
    uint64_t quantumStartFrame = context->currentSampleFrame();
    uint64_t quantumEndFrame = quantumStartFrame + quantumFrameSize;
    uint64_t startFrame = AudioUtilities::timeToSampleFrame(m_startTime, sampleRate);
    uint64_t endFrame = m_endTime == UNKNOWN_TIME ? 0 : AudioUtilities::timeToSampleFrame(m_endTime, sampleRate);

    // If end time is known and it's already passed, then don't do any more rendering
    if (m_endTime != UNKNOWN_TIME && endFrame <= quantumStartFrame)
    {
        finish(r);
    }

    // If unscheduled, or finished, or out of time, output silence
    if (m_playbackState == UNSCHEDULED_STATE || m_playbackState == FINISHED_STATE || startFrame >= quantumEndFrame)
    {
        // Output silence.
        outputBus->zero();
        nonSilentFramesToProcess = 0;
        return;
    }

    // Check if it's time to start playing.
    if (m_playbackState == SCHEDULED_STATE)
    {
        m_playbackState = PLAYING_STATE;
    }

    quantumFrameOffset = startFrame > quantumStartFrame ? startFrame - quantumStartFrame : 0;
    quantumFrameOffset = std::min(quantumFrameOffset, quantumFrameSize);  // clamp to valid range
    nonSilentFramesToProcess = quantumFrameSize - quantumFrameOffset;

    if (!nonSilentFramesToProcess)
    {
        // Output silence.
        outputBus->zero();
        return;
    }

    // Handle silence before we start playing.
    // Zero any initial frames representing silence leading up to a rendering start time in the middle of the quantum.
    if (quantumFrameOffset)
    {
        for (int i = 0; i < outputBus->numberOfChannels(); ++i)
        {
            memset(outputBus->channel(i)->mutableData(), 0, sizeof(float) * quantumFrameOffset);
        }
    }

    // Handle silence after we're done playing.
    // If the end time is somewhere in the middle of this time quantum, then zero out the
    // frames from the end time to the very end of the quantum.
    if (m_endTime != UNKNOWN_TIME && endFrame >= quantumStartFrame && endFrame < quantumEndFrame)
    {
        size_t zeroStartFrame = endFrame - quantumStartFrame;
        size_t framesToZero = quantumFrameSize - zeroStartFrame;

        bool isSafe = zeroStartFrame < quantumFrameSize && framesToZero <= quantumFrameSize && zeroStartFrame + framesToZero <= quantumFrameSize;
        ASSERT(isSafe);

        if (isSafe)
        {
            if (framesToZero > nonSilentFramesToProcess) nonSilentFramesToProcess = 0;
            else nonSilentFramesToProcess -= framesToZero;

            for (int i = 0; i < outputBus->numberOfChannels(); ++i)
            {
                memset(outputBus->channel(i)->mutableData() + zeroStartFrame, 0, sizeof(float) * framesToZero);
            }
        }

        finish(r);
    }
}

void SampledAudioVoice::schedule(double when, double grainOffset)
{
    // Duration of 0 is special (calculate based on the entire buffer's duration)
    schedule(when, grainOffset, 0);
}

void SampledAudioVoice::schedule(double when, double grainOffset, double grainDuration)
{
    m_requestWhen = when;
    m_requestGrainOffset = grainOffset;
    m_requestGrainDuration = grainDuration;
    m_playbackState = SCHEDULED_STATE;
    m_startRequested = true;
}

bool SampledAudioVoice::renderSilenceAndFinishIfNotLooping(ContextRenderLock & r, AudioBus * bus, size_t index, size_t framesToProcess)
{
    if (!m_isLooping->valueBool())
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

bool SampledAudioVoice::renderSample(ContextRenderLock & r, AudioBus * bus, size_t destinationSampleOffset, size_t frameSize)
{
    if (!r.context())
        return false;

    // @fixme - render loop shared_ptr retrieval (extremely slow) 
    std::shared_ptr<AudioBus> srcBus = m_sourceBus->valueBus();

    if (!bus || !srcBus)
        return false;

    size_t numChannels = numberOfChannels(r);
    size_t busNumberOfChannels = bus->numberOfChannels();

    bool channelCountGood = numChannels && numChannels == busNumberOfChannels;
    ASSERT(channelCountGood);
    if (!channelCountGood)
        return false;

    // Sanity check destinationSampleOffset, frameSize.
    size_t destinationLength = bus->length();

    bool isLengthGood = destinationLength <= 4096 && frameSize <= 4096;
    ASSERT(isLengthGood);
    if (!isLengthGood)
        return false;

    bool isOffsetGood = (destinationSampleOffset <= destinationLength) && ((destinationSampleOffset + frameSize) <= destinationLength);
    ASSERT(isOffsetGood);
    if (!isOffsetGood)
        return false;

    // Offset the pointers to the correct offset frame.
    size_t writeIndex = destinationSampleOffset;

    size_t bufferLength = srcBus->length();
    double bufferSampleRate = srcBus->sampleRate();

    // Avoid converting from time to sample-frames twice by computing
    // the grain end time first before computing the sample frame.
    size_t endFrame = m_isGrain ? AudioUtilities::timeToSampleFrame(m_grainOffset + m_grainDuration, bufferSampleRate) : bufferLength;

    // This is a HACK to allow for HRTF tail-time - avoids glitch at end.
    // FIXME: implement tailTime for each AudioNode for a more general solution to this problem.
    // https://bugs.webkit.org/show_bug.cgi?id=77224
    if (m_isGrain) endFrame += 512;

    // Do some sanity checking.
    if (endFrame > bufferLength) endFrame = bufferLength;
    if (m_virtualReadIndex >= endFrame) m_virtualReadIndex = 0; // reset to start

    // If the .loop attribute is true, then values of m_loopStart == 0 && m_loopEnd == 0 implies
    // that we should use the entire buffer as the loop, otherwise use the loop values in m_loopStart and m_loopEnd.
    double virtualEndFrame = static_cast<double>(endFrame);
    double virtualDeltaFrames = virtualEndFrame;

    double loopS = m_loopStart->valueFloat();
    double loopE = m_loopEnd->valueFloat();

    if (m_isLooping->valueBool() && (loopS || loopE) && loopS >= 0 && loopE > 0 && loopS < loopE)
    {
        // Convert from seconds to sample-frames.
        double loopStartFrame = loopS * srcBus->sampleRate();
        double loopEndFrame = loopE * srcBus->sampleRate();

        virtualEndFrame = std::min(loopEndFrame, virtualEndFrame);
        virtualDeltaFrames = virtualEndFrame - loopStartFrame;
    }

    // Sanity check that our playback rate isn't larger than the loop size.
    if (std::abs(m_totalPitchRate) >= (float) virtualDeltaFrames)
        return false;

    // Get local copy.
    double virtualReadIndex = m_virtualReadIndex;

    // Render loop - reading from the source buffer to the destination using linear interpolation.
    int samplesToProcess = static_cast<int>(frameSize);

    // Optimize for the very common case of playing back with pitchRate == 1.
    // We can avoid the linear interpolation.
    if (m_totalPitchRate == 1 && virtualReadIndex == floor(virtualReadIndex) && virtualDeltaFrames == floor(virtualDeltaFrames) && virtualEndFrame == floor(virtualEndFrame))
    {
        int readIndex = static_cast<int>(virtualReadIndex);
        int deltaFrames = static_cast<int>(virtualDeltaFrames);
        endFrame = static_cast<int>(virtualEndFrame);

        while (samplesToProcess > 0)
        {
            int framesToEnd = static_cast<int>(endFrame) - readIndex;
            int framesThisTime = std::min(samplesToProcess, framesToEnd);
            framesThisTime = std::max(0, framesThisTime);

            for (unsigned i = 0; i < numChannels; ++i)
            {
                for (int sample = 0; sample < framesThisTime; ++sample)
                {
                    memcpy(bus->channel(i)->mutableData() + writeIndex, srcBus->channel(i)->data() + readIndex, sizeof(float) * framesThisTime);
                }
            }

            writeIndex += framesThisTime;
            readIndex += framesThisTime;
            samplesToProcess -= framesThisTime;

            // wrap-around, retaining sub-sample position since virtualReadIndex is floating-point
            if (readIndex >= endFrame)
            {
                readIndex -= deltaFrames;
                if (renderSilenceAndFinishIfNotLooping(r, bus, static_cast<unsigned int>(writeIndex), static_cast<size_t>(samplesToProcess)))
                {
                    break;
                }
            }
        }
        virtualReadIndex = readIndex;
    }
    else
    {
        while (samplesToProcess--)
        {
            unsigned readIndex = static_cast<unsigned>(virtualReadIndex);
            double interpolationFactor = virtualReadIndex - readIndex;

            // For linear interpolation we need the next sample-frame too.
            unsigned readIndex2 = readIndex + 1;

            if (readIndex2 >= bufferLength)
            {
                // Make sure to wrap around at the end of the buffer.
                if (m_isLooping->valueBool())
                {
                    readIndex2 = static_cast<unsigned>(virtualReadIndex + 1 - virtualDeltaFrames);
                }
                else readIndex2 = readIndex;
            }

            // Final sanity check on buffer access.
            // FIXME: as an optimization, try to get rid of this inner-loop check and put assertions and guards before the loop.
            if (readIndex >= bufferLength || readIndex2 >= bufferLength) break;

            // Linear interpolation.
            for (unsigned i = 0; i < numChannels; ++i)
            {
                float * destination = bus->channel(i)->mutableData();
                const float * source = srcBus->channel(i)->data();

                double sample1 = source[readIndex];
                double sample2 = source[readIndex2];
                double sample = (1.0 - interpolationFactor) * sample1 + interpolationFactor * sample2;

                destination[writeIndex] = static_cast<float>(sample);
            }
            writeIndex++;

            virtualReadIndex += m_totalPitchRate;

            // wrap-around, retaining sub-sample position since virtualReadIndex is floating-point
            if (virtualReadIndex >= virtualEndFrame)
            {
                virtualReadIndex -= virtualDeltaFrames;
                if (renderSilenceAndFinishIfNotLooping(r, bus, writeIndex, static_cast<size_t>(samplesToProcess)))
                {
                    break;
                }
            }
        }
    }

    bus->clearSilentFlag();

    m_virtualReadIndex = virtualReadIndex;

    return true;
}

void SampledAudioVoice::process(ContextRenderLock & r, int framesToProcess)
{
    if (!m_sourceBus->valueBus() || !isInitialized() || !r.context() || !m_inPlaceBus)
    {
        m_inPlaceBus->zero();
        return;
    }

    if (m_channelSetupRequested)
    {
        // channel count is changing, so output silence for one quantum to allow the
        // context to re-evaluate connectivity before rendering
        m_inPlaceBus->zero();
        m_output->setNumberOfChannels(r, m_sourceBus->valueBus()->numberOfChannels());
        m_virtualReadIndex = 0;
        m_channelSetupRequested = false;
        return;
    }

    // After calling setBuffer() with a buffer having a different number of channels, there can in rare cases be a slight delay
    // before the output bus is updated to the new number of channels because of use of tryLocks() in the context's updating system.
    // In this case, if the the buffer has just been changed and we're not quite ready yet, then just output silence.
    if (numberOfChannels(r) != m_sourceBus->valueBus()->numberOfChannels())
    {
        m_inPlaceBus->zero();
        return;
    }

    if (m_startRequested)
    {
        double grainOffset = std::max(0.0, m_requestGrainOffset);
        m_grainOffset = std::min((double)m_duration, grainOffset);
        m_grainOffset = grainOffset;

        // Handle default/unspecified duration.
        double maxDuration = m_duration - grainOffset;
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

        // @todo consistently pick double or size_t through this entire API chain.
        m_virtualReadIndex = static_cast<double>(AudioUtilities::timeToSampleFrame(m_grainOffset, static_cast<double>(m_sourceBus->valueBus()->sampleRate())));
        m_startRequested = false;
    }

    // Update sample-accurate scheduling
    size_t quantumFrameOffset;
    size_t bufferFramesToProcess;
    updateSchedulingInfo(r, framesToProcess, m_inPlaceBus.get(), quantumFrameOffset, bufferFramesToProcess);

    if (!bufferFramesToProcess)
    {
        m_inPlaceBus->zero();
        return;
    }

    // Write sample into buffer
    if (!renderSample(r, m_inPlaceBus.get(), quantumFrameOffset, bufferFramesToProcess))
    {
        m_inPlaceBus->zero();
        return;
    }

    // Apply the gain (in-place) to the output bus.
    float totalGain = m_gain->value(r);
    m_inPlaceBus->copyWithGainFrom(*m_inPlaceBus, &m_lastGain, totalGain);
}

void SampledAudioVoice::reset(ContextRenderLock & r)
{
    m_lastGain = m_gain->value(r);
    m_virtualReadIndex = 0;
    m_pendingEndTime = UNKNOWN_TIME;
    m_playbackState = UNSCHEDULED_STATE;
}

void SampledAudioVoice::finish(ContextRenderLock& r)
{
    m_playbackState = FINISHED_STATE;
    if (m_onEnded) r.context()->enqueueEvent(m_onEnded);
}

//////////////////////////
//   SampledAudioNode   //
//////////////////////////

SampledAudioNode::SampledAudioNode(AudioContext & ac) : AudioNode(ac)
{
    m_sourceBus = std::make_shared<AudioSetting>("sourceBus", "SBUS", AudioSetting::Type::Bus);
    m_isLooping = std::make_shared<AudioSetting>("loop", "LOOP", AudioSetting::Type::Bool);
    m_loopStart = std::make_shared<AudioSetting>("loopStart", "LSTR", AudioSetting::Type::Float);
    m_loopEnd = std::make_shared<AudioSetting>("loopEnd", "LEND ", AudioSetting::Type::Float);

    m_gain = std::make_shared<AudioParam>("gain", "GAIN", 1.0, 0.0, 1.0);
    m_playbackRate = std::make_shared<AudioParam>("playbackRate", "RATE", 1.0, 0.0, 1024);
    m_detune = std::make_shared<AudioParam>("detune", "DTUNE", 0.0, 0.0, 1200.f);

    m_params.push_back(m_gain);
    m_params.push_back(m_playbackRate);
    m_params.push_back(m_detune);

    m_settings.push_back(m_sourceBus);
    m_settings.push_back(m_isLooping);
    m_settings.push_back(m_loopStart);
    m_settings.push_back(m_loopEnd);

    m_sourceBus->setValueChanged([this]() {
        for (auto & voice : voices) voice->m_channelSetupRequested = true;
    });

    // Default to mono. A call to setBus() will set the number of output channels to that of the bus.
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

    initialize();
}

SampledAudioNode::~SampledAudioNode()
{
    if (isInitialized()) uninitialize();
}

void SampledAudioNode::process(ContextRenderLock & r, int framesToProcess)
{
    if (m_onEnded)
    {
        for (auto & v : voices)
        {
            v->setOnEnded(m_onEnded);
        }
    }

    std::stack<SampledAudioVoice *> free_voices;
    for (const auto & v : voices)
    {
        if (v->isPlayingOrScheduled()) continue; // voice is occupied
        else if (v->playbackState() == SampledAudioVoice::UNSCHEDULED_STATE ||
                 v->playbackState() == SampledAudioVoice::FINISHED_STATE) { free_voices.push(v.get()); }
    }

    if (free_voices.size())
    {
        // We can pre-schedule up to num_voices into the future by cycling through the nodes. 
        auto num_to_schedule = std::min(schedule_list.size(), free_voices.size()); // whichever is smaller (many, MAX_NUM_VOICES)

        double now = r.context()->currentTime();
        while (num_to_schedule > 0)
        {
            auto grain_to_schedule = schedule_list.front();
            schedule_list.pop_front();
            free_voices.top()->schedule(grain_to_schedule.when + now, grain_to_schedule.grainOffset, grain_to_schedule.grainDuration);
            free_voices.pop();
            num_to_schedule--;
        }
    }

    for (auto & v : voices) 
    {
        v->setPitchRate(r,(float) totalPitchRate(r));
        v->process(r, framesToProcess);
    }

    auto outputBus = output(0)->bus(r);
    const int numberOfChannels = outputBus->numberOfChannels();

    if (!m_summingBus)
    {
        m_summingBus.reset(new AudioBus(numberOfChannels, outputBus->length()));
    }

    m_summingBus->zero();

    for (const auto & v : voices)
    {
        if (v->isPlayingOrScheduled()) 
        {
            m_summingBus->sumFrom(*(v->m_inPlaceBus));
        }
    }

    outputBus->copyFrom(*m_summingBus, ChannelInterpretation::Discrete);
}

void SampledAudioNode::reset(ContextRenderLock & r)
{
   for (auto & v : voices) 
   {
       v->reset(r);
   }

   voices.clear();
}

bool SampledAudioNode::setBus(ContextRenderLock & r, std::shared_ptr<AudioBus> buffer)
{
    ASSERT(r.context());

    m_sourceBus->setBus(buffer.get());
    output(0)->setNumberOfChannels(r, buffer ? buffer->numberOfChannels() : 0);

    // voice creation
    for (size_t v = 0; v < MAX_NUM_VOICES; ++v)
    {
        std::unique_ptr<SampledAudioVoice> voice(new SampledAudioVoice(*r.context(), duration(), m_gain, m_playbackRate, m_detune, m_isLooping, m_loopStart, m_loopEnd, m_sourceBus));
        voice->setOutput(r, output(0));
        voices.push_back(std::move(voice));
    }

    return true;
}

void SampledAudioNode::schedule(double when, double grainOffset)
{
    schedule(when, grainOffset, 0.0);
}

void SampledAudioNode::schedule(double when, double grainOffset, double grainDuration)
{
    schedule_list.emplace_back(when, grainOffset, 0.0);
    schedule_list.unique(); // remove duplicate schedules
    schedule_list.sort(); // sort by time
}

float SampledAudioNode::duration() const
{
    auto bus = getBus();
    if (!bus) return 0;
    return bus->length() / bus->sampleRate();
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
    totalRate *= pow(2, detune()->value(r) / 1200);

    // Sanity check the total rate.  It's very important that the resampler not get any bad rate values.
    totalRate = std::max(0.0, totalRate);
    if (!totalRate)
        totalRate = 1;  // zero rate is considered illegal
    totalRate = std::min(1024.0, totalRate);

    bool isTotalRateValid = !std::isnan(totalRate) && !std::isinf(totalRate);
    ASSERT(isTotalRateValid);
    if (!isTotalRateValid)
        totalRate = 1.0;

    return totalRate;
}

void SampledAudioNode::setPannerNode(PannerNode * pannerNode)
{
    m_pannerNode = pannerNode;
}

void SampledAudioNode::clearPannerNode()
{
    m_pannerNode = nullptr;
}

bool SampledAudioNode::propagatesSilence(ContextRenderLock & r) const
{
   return false;
}
