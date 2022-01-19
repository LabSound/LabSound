// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/Macros.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"

#include "concurrentqueue/concurrentqueue.h"

#include <algorithm>

using namespace lab;

AudioBus const* const AudioParam::bus() const
{
    return _input.summingBus;
}


const double AudioParam::DefaultSmoothingConstant = 0.05;
const double AudioParam::SnapThreshold = 0.001;

AudioParam::AudioParam(AudioParamDescriptor const * const desc) noexcept
    : _desc(desc)
    , m_value(desc->defaultValue)
    , m_smoothedValue(desc->defaultValue)
    , m_smoothingConstant(DefaultSmoothingConstant)
    , _input("driver")
{
    _work = (ConcurrentQueue *) new moodycamel::ConcurrentQueue<Work>();
}

AudioParam::~AudioParam() 
{
    delete (moodycamel::ConcurrentQueue<Work> *) _work;
}

float AudioParam::value() const
{
    return static_cast<float>(m_value);
}

void AudioParam::setValue(float value)
{
    if (!std::isnan(value) && !std::isinf(value))
        m_value = value;
}

float AudioParam::smoothedValue()
{
    return static_cast<float>(m_smoothedValue);
}

bool AudioParam::smooth(ContextRenderLock & r)
{
    // If values have been explicitly scheduled on the timeline, then use the exact value.
    // Smoothing effectively is performed by the timeline.
    bool useTimelineValue = false;
    if (r.context())
        m_value = m_timeline.valueForContextTime(r, static_cast<float>(m_value), useTimelineValue);

    if (m_smoothedValue == m_value)
    {
        // Smoothed value has already approached and snapped to value.
        return true;
    }

    if (useTimelineValue)
        m_smoothedValue = m_value;
    else
    {
        // Dezipper - exponential approach.
        m_smoothedValue += (m_value - m_smoothedValue) * m_smoothingConstant;

        // If we get close enough then snap to actual value.
        if (fabs(m_smoothedValue - m_value) < SnapThreshold)  // @fixme: the threshold needs to be adjustable depending on range - but this is OK general purpose value.
            m_smoothedValue = m_value;
    }

    return false;
}

float AudioParam::finalValue(ContextRenderLock & r)
{
    float value;
    calculateFinalValues(r, &value, 1, false);
    return value;
}

bool AudioParam::hasSampleAccurateValues()
{
    moodycamel::ConcurrentQueue<Work> * workQueue = (moodycamel::ConcurrentQueue<Work> *) _work;
    return m_timeline.hasValues() || _input.sources.size() > 0 || workQueue->size_approx() > 0;
}

void AudioParam::calculateSampleAccurateValues(ContextRenderLock & r, 
    float * values, int numberOfValues)
{
    bool isSafe = r.context() && values && numberOfValues;
    if (!isSafe)
        return;

    calculateFinalValues(r, values, numberOfValues, true);
}

void AudioParam::serviceQueue(ContextRenderLock & r)
{
    moodycamel::ConcurrentQueue<Work> * workQueue = (moodycamel::ConcurrentQueue<Work> *) _work;
    if (workQueue->size_approx() > 0)
    {
        Work w;
        while (workQueue->try_dequeue(w))
        {
            switch (w.op)
            {
                case 0:
                    _input.clear();
                    break;

                case 1:
                    _input.sources.emplace_back(AudioNodeSummingInput::Source {w.node, w.output});
                    break;

                case -1:
                {
                    int src_count = (int) _input.sources.size();
                    for (int i = 0; i < src_count; ++i)
                    {
                        if (_input.sources[i] && _input.sources[i]->node.get() == w.node.get())
                        {
                            delete _input.sources[i];
                            _input.sources[i] = nullptr;
                        }
                    }
                }
            }
        }
    }
}


void AudioParam::calculateFinalValues(ContextRenderLock & r, 
    float * values, int numberOfValues, bool sampleAccurate)
{
    bool isSafe = r.context() && values && numberOfValues;
    if (!isSafe)
        return;

    // The calculated result will be the "intrinsic" value summed with all audio-rate connections.

    if (sampleAccurate)
    {
        // Calculate sample-accurate (a-rate) intrinsic values.
        calculateTimelineValues(r, values, numberOfValues);
    }
    else
    {
        // Calculate control-rate (k-rate) intrinsic value.
        bool hasValue;
        float timelineValue = m_timeline.valueForContextTime(r, static_cast<float>(m_value), hasValue);

        if (hasValue)
            m_value = timelineValue;

        values[0] = static_cast<float>(m_value);
    }

    // Now sum all of the audio-rate connections together (unity-gain summing junction).
    // Note that parameter connections would normally be mono, so mix down to mono if necessary.

    _input.updateSummingBus(1, numberOfValues);
    _input.summingBus->copyChannelMemory(0, values, numberOfValues);

    for (auto i : _input.sources)
    {
        if (!i)
            continue;

        auto output = i->node->outputBus(r, i->out);
        if (!output)
            continue;

        // audio node was preprocessed in the root recursion

        // Sum, with unity-gain.
        /// @TODO it was surprising in practice that the inputs are summed, as opposed to simply overriding.
        /// Summing might be useful, but pure override should be an option as well.
        /// The case in point was to construct a vibrato around A440 by making an oscillator provide
        /// a signal with frequency 4, bias 440, amplitude 10, and supply that as an override to the frequency of
        /// a second oscillator. Since it's summed, the solution that works is that the first oscillator should
        /// have a bias of zero. It seems like sum or override should be a setting of some sort...
        _input.summingBus->sumFrom(*i->node->outputBus(r, 0));
    }
}

void AudioParam::calculateTimelineValues(ContextRenderLock & r, 
    float * values, int numberOfValues)
{
    // Calculate values for this render quantum.
    // Normally numberOfValues will equal AudioNode::ProcessingSizeInFrames 
    // (the render quantum size).
    double sampleRate = r.context()->sampleRate();
    double startTime = r.context()->currentTime();
    double endTime = startTime + numberOfValues / sampleRate;

    // Note we're running control rate at the sample-rate.
    // Pass in the current value as default value.
    m_value = m_timeline.valuesForTimeRange(startTime, endTime, 
        static_cast<float>(m_value), values, numberOfValues, sampleRate, sampleRate);
}

void AudioParam::connect(std::shared_ptr<AudioNode> n, int output)
{
    if (!n)
        return;

    moodycamel::ConcurrentQueue<Work> * workQueue = (moodycamel::ConcurrentQueue<Work> *) _work;
    workQueue->enqueue(Work {n, output, 1});
}

// -1 for output means any found connections
void AudioParam::disconnect(std::shared_ptr<AudioNode> n, int output)
{
    if (!n)
        return;

    moodycamel::ConcurrentQueue<Work> * workQueue = (moodycamel::ConcurrentQueue<Work> *) _work;
    workQueue->enqueue(Work {n, output, -1});
}

void AudioParam::disconnectAll()
{
    moodycamel::ConcurrentQueue<Work> * workQueue = (moodycamel::ConcurrentQueue<Work> *) _work;
    workQueue->enqueue(Work {{}, 0, 0});
}
