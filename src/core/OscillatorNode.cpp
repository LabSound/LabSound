// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/OscillatorNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"
#include "LabSound/core/WaveTable.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"
#include "internal/VectorMath.h"

#include <algorithm>
#include <emmintrin.h>

using namespace lab;

// https://www.musicdsp.org/en/latest/Synthesis/13-sine-calculation.html
// phase should be between between -LAB_PI and +LAB_PI
inline float burk_fast_sine(const double phase) 
{
    // Factorial coefficients.
    const float IF3 = 1.f / (2 * 3);
    const float IF5 = IF3 / (4 * 5);
    const float IF7 = IF5 / (6 * 7);
    const float IF9 = IF7 / (8 * 9);
    const float IF11 = IF9 / (10 * 11);

    // Wrap phase back into region where results are more accurate. 
    double x = (phase > +LAB_HALF_PI) ? +(LAB_PI - phase)
           : ((phase < -LAB_HALF_PI) ? -(LAB_PI + phase) : phase);

    double x2 = (x * x);

    // Taylor expansion out to x**11/11! factored into multiply-adds 
    return static_cast<float>(
            x * (x2 * (x2 * (x2 * (x2 * ((x2 * (-IF11)) + IF9) - IF7) + IF5) - IF3) + 1)
        );
}

static char const * const s_types[] = {"None", "Sine", "FastSine", "Square", "Sawtooth", "Triangle", "Custom", nullptr};

OscillatorNode::OscillatorNode(AudioContext & ac)
    : AudioScheduledSourceNode(ac)
    , m_phaseIncrements(AudioNode::ProcessingSizeInFrames)
    , m_detuneValues(AudioNode::ProcessingSizeInFrames)
{
using namespace VectorMath;

    void OscillatorNode::process_oscillator(ContextRenderLock & r, int framesToProcess)
    {
        outputBus->zero();
        return;
    }

    if (framesToProcess > m_phaseIncrements.size()) m_phaseIncrements.allocate(framesToProcess);
    if (framesToProcess > m_detuneValues.size()) m_detuneValues.allocate(framesToProcess);
    if (framesToProcess > m_amplitudeValues.size()) m_amplitudeValues.allocate(framesToProcess);
    if (framesToProcess > m_biasValues.size()) m_biasValues.allocate(framesToProcess);

    if (bufferSize > m_phaseIncrements.size()) m_phaseIncrements.allocate(bufferSize);
    if (bufferSize > m_detuneValues.size()) m_detuneValues.allocate(bufferSize);
    if (bufferSize > m_amplitudeValues.size()) m_amplitudeValues.allocate(bufferSize);
    if (bufferSize > m_biasValues.size()) m_biasValues.allocate(bufferSize);


    // calculate phase increments
    float* phaseIncrements = m_phaseIncrements.data();

    if (m_frequency->hasSampleAccurateValues())
    {
        // Get the sample-accurate frequency values in preparation for conversion to phase increments.
        // They will be converted to phase increments below.
        m_frequency->calculateSampleAccurateValues(r, phaseIncrements, nonSilentFramesToProcess);
    }
    else
    {
        // Handle ordinary parameter smoothing/de-zippering if there are no scheduled changes.
        m_frequency->smooth(r);
        float frequency = m_frequency->smoothedValue();
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
        {
            phaseIncrements[i] = frequency;
        }
    }

    // convert frequencies to phase increments
    for (int i = 0; i < framesToProcess; ++i)
    {
        phaseIncrements[i] = static_cast<float>(2.f * M_PI * phaseIncrements[i] / m_sampleRate);
    }

    // fetch the amplitudes
    float * amplitudes = m_amplitudeValues.data();
    if (m_amplitude->hasSampleAccurateValues())
    {
        m_amplitude->calculateSampleAccurateValues(r, amplitudes, framesToProcess);
    }
    else
    {
        m_amplitude->smooth(r);
        float amp = m_amplitude->smoothedValue();
        for (int i = 0; i < framesToProcess; ++i)
            amplitudes[i] = amp;
    }

    // fetch the bias values
    float * bias = m_biasValues.data();
    if (m_bias->hasSampleAccurateValues())
    {
        m_bias->calculateSampleAccurateValues(r, bias, framesToProcess);
    }
    else
    {
        m_bias->smooth(r);
        float b = m_bias->smoothedValue();
        for (int i = 0; i < framesToProcess; ++i)
            bias[i] = b;
    }

    // calculate and write the wave
    float * destP = outputBus->channel(0)->mutableData();

    OscillatorType type = static_cast<OscillatorType>(m_type->valueUint32());
    switch (type)
    {
        case OscillatorType::SINE:
        {
            // Get the sample-accurate detune values.
            float * detuneValues = m_detuneValues.data();
            m_detune->calculateSampleAccurateValues(r, detuneValues, framesToProcess);

        // Convert from cents to rate scalar and perform detuning
        float k = 1.f / 1200.f;
        VectorMath::vsmul(offset_detunes, 1, &k, offset_detunes, 1, nonSilentFramesToProcess);
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
        {
            phaseIncrements[i] *= powf(2, detuneValues[i]);  // FIXME: converting to expf() will be faster.
        }
    }
    else
    {
        // Handle ordinary parameter smoothing/de-zippering if there are no scheduled changes.
        m_detune->smooth(r);
        float detune = m_detune->smoothedValue();
        if (fabsf(detune) > 0.01f)
        {
            float detuneScale = powf(2, detune / 1200);
            for (int i = 0; i < framesToProcess; ++i)
            {
                float amp = amplitudes[i];
                destP[i] = static_cast<float>(bias[i] + (_lab_phase < M_PI ? amp : -amp));
                _lab_phase += phaseIncrements[i];
                if (_lab_phase > 2. * M_PI)
                    _lab_phase -= 2. * M_PI;
            }
        }
        break;

        case OscillatorType::SAWTOOTH:
        {
            for (int i = 0; i < framesToProcess; ++i)
            {
                float amp = amplitudes[i];
                destP[i] = static_cast<float>(bias[i] + amp - (amp / M_PI * _lab_phase));
                _lab_phase += phaseIncrements[i];
                if (_lab_phase > 2. * M_PI)
                    _lab_phase -= 2. * M_PI;
            }
        }
        break;

        case OscillatorType::TRIANGLE:
        {
            for (int i = 0; i < framesToProcess; ++i)
            {
                float amp = amplitudes[i];
                if (_lab_phase < M_PI)
                    destP[i] = static_cast<float>(bias[i] - amp + (2.f * amp / float(M_PI)) * _lab_phase);
                else
                    destP[i] = static_cast<float>(bias[i] + 3.f * amp - (2.f * amp / float(M_PI)) * _lab_phase);

                _lab_phase += phaseIncrements[i];
                if (_lab_phase > 2. * M_PI)
                    _lab_phase -= 2. * M_PI;
            }
        }
        break;
    }

    outputBus->clearSilentFlag();
}

/// @TODO delete these, and only have a single wave table
/// The interface should be that the user supplies a Bus, and then the wave table
/// will be computed from the bus.

std::shared_ptr<WaveTable> OscillatorNode::s_waveTableSine = 0;
std::shared_ptr<WaveTable> OscillatorNode::s_waveTableSquare = 0;
std::shared_ptr<WaveTable> OscillatorNode::s_waveTableSawtooth = 0;
std::shared_ptr<WaveTable> OscillatorNode::s_waveTableTriangle = 0;

static char const * const s_types[OscillatorType::_OscillatorCount + 1] = {
    "None", "Sine", "Square", "Sawtooth", "Triangle", "Custom",
    nullptr};

OscillatorNode::OscillatorNode(const float sampleRate)
    : m_sampleRate(sampleRate)
    , m_type(std::make_shared<AudioSetting>("type", "TYPE", s_types))
    , m_firstRender(true)
    , m_virtualReadIndex(0)
    , m_phaseIncrements(AudioNode::ProcessingSizeInFrames)
    , m_detuneValues(AudioNode::ProcessingSizeInFrames)
{
    // Use musical pitch standard A440 as a default.
    m_frequency = std::make_shared<AudioParam>("frequency", "FREQ", 440, 0, 100000);

    // convert frequencies to phase increments
    for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
    {
        phaseIncrements[i] = static_cast<float>(2.f * static_cast<float>(LAB_PI)* phaseIncrements[i] / sample_rate);
    }

    // fetch the amplitudes
    float* amplitudes = m_amplitudeValues.data();
    if (m_amplitude->hasSampleAccurateValues())
    {
        m_amplitude->calculateSampleAccurateValues(r, amplitudes + quantumFrameOffset, nonSilentFramesToProcess);
    }
    else
    {
        m_amplitude->smooth(r);
        float amp = m_amplitude->smoothedValue();
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
            amplitudes[i] = amp;
    }

    // fetch the bias values
    float* bias = m_biasValues.data();
    if (m_bias->hasSampleAccurateValues())
    {
        m_bias->calculateSampleAccurateValues(r, bias + quantumFrameOffset, nonSilentFramesToProcess);
    }
    else
    {
        m_bias->smooth(r);
        float b = m_bias->smoothedValue();
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
        {
            bias[i] = b;
        }
    }

    // calculate and write the wave
    float* destP = outputBus->channel(0)->mutableData();

    OscillatorType type = static_cast<OscillatorType>(m_type->valueUint32());
    switch (type)
    {
    case OscillatorType::SINE:
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
        {
            destP[i] = static_cast<float>(bias[i] + amplitudes[i] * static_cast<float>(sin(phase)));
            phase += phaseIncrements[i];
            if (phase > 2. * static_cast<float>(LAB_PI)) phase -= 2. * static_cast<float>(LAB_PI);
        }
        break;

void OscillatorNode::process(ContextRenderLock & r, size_t framesToProcess)
{
    OscillatorType type = static_cast<OscillatorType>(m_type->valueUint32());
    if (type != OscillatorType::CUSTOM)
    {
        process_oscillator(r, static_cast<int>(framesToProcess));
        return;
    }
    return;

    /// @TODO add interface to allow a user to set the wavetable

        float finalScale = m_waveTable->rateScale();

    AudioBus * outputBus = output(0)->bus(r);

            // Get the sample-accurate frequency values in preparation for conversion to phase increments.
            // They will be converted to phase increments below.
            m_frequency->calculateSampleAccurateValues(r, phaseIncrements, framesToProcess);
        }
        break;

    case OscillatorType::SQUARE:
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
        {
            float amp = amplitudes[i];
            destP[i] = static_cast<float>(bias[i] + (phase < static_cast<float>(LAB_PI) ? amp : -amp));
            phase += phaseIncrements[i];
            if (phase > 2. * static_cast<float>(LAB_PI))
                phase -= 2. * static_cast<float>(LAB_PI);
        }
        break;

    case OscillatorType::SAWTOOTH:
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
        {
            float amp = amplitudes[i];
            destP[i] = static_cast<float>(bias[i] + amp - (amp / static_cast<float>(LAB_PI)* phase));
            phase += phaseIncrements[i];
            if (phase > 2. * static_cast<float>(LAB_PI))
                phase -= 2. * static_cast<float>(LAB_PI);
        }
        break;

    case OscillatorType::TRIANGLE:
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
        {
            for (int i = 0; i < framesToProcess; ++i)
            {
                float amp = amplitudes[i];
                if (phase < static_cast<float>(LAB_PI))
                    destP[i] = static_cast<float>(bias[i] - amp + (2.f * amp / static_cast<float>(LAB_PI)) * phase);
                else
                    destP[i] = static_cast<float>(bias[i] + 3.f * amp - (2.f * amp / static_cast<float>(LAB_PI)) * phase);
       
                phase += phaseIncrements[i];
                if (phase > 2. * static_cast<float>(LAB_PI))
                    phase -= 2. * static_cast<float>(LAB_PI);
            }
        }
        break;
    }

            phase += phaseIncrements[i];
            if (phase > 2. * static_cast<float>(LAB_PI))
                phase -= 2. * static_cast<float>(LAB_PI);
        }
        break;
            
    default: break; // other types do nothing
    }

    outputBus->clearSilentFlag();
}

static char const * const s_types[OscillatorType::_OscillatorCount + 1] = {"None", "Sine", "Square", "Sawtooth", "Triangle", "Custom", nullptr};

OscillatorNode::OscillatorNode(const float sampleRate)
    : m_sampleRate(sampleRate), m_phaseIncrements(AudioNode::ProcessingSizeInFrames), m_detuneValues(AudioNode::ProcessingSizeInFrames)
{
    m_type      = std::make_shared<AudioSetting>("type", "TYPE", s_types);
    m_frequency = std::make_shared<AudioParam>("frequency", "FREQ", 440, 0, 100000);
    m_detune    = std::make_shared<AudioParam>("detune", "DTUN", 0, -4800, 4800);
    m_amplitude = std::make_shared<AudioParam>("amplitude", "AMPL", 1, 0, 100000);
    m_bias      = std::make_shared<AudioParam>("bias", "BIAS", 0, -1000000, 100000);

    m_params.push_back(m_frequency);
    m_params.push_back(m_amplitude);
    m_params.push_back(m_bias);
    m_params.push_back(m_detune);

    m_type->setValueChanged([this]() { _setType(OscillatorType(m_type->valueUint32())); });
    m_settings.push_back(m_type);

    // Sets up default wavetable.
    setType(OscillatorType::SINE);

    // An oscillator is always mono.
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

    initialize();
}

OscillatorNode::~OscillatorNode()
{
    uninitialize();
}

OscillatorType OscillatorNode::type() const
{
    return OscillatorType(m_type->valueUint32());
}

void OscillatorNode::setType(OscillatorType type)
{
    m_type->setUint32(static_cast<uint32_t>(type));
}

void OscillatorNode::_setType(OscillatorType type)
{
    if (type == OscillatorType::CUSTOM) return;  // @todo - not yet implemented
    m_type->setUint32(static_cast<uint32_t>(type), false);
}

        if (hasSampleAccurateValues)
        {
            incr = *phaseIncrements++;

            frequency = invRateScale * incr;
            m_waveTable->waveDataForFundamentalFrequency(frequency, lowerWaveData, higherWaveData, tableInterpolationFactor);
        }

        float sample1Lower = lowerWaveData[readIndex];
        float sample2Lower = lowerWaveData[readIndex2];
        float sample1Higher = higherWaveData[readIndex];
        float sample2Higher = higherWaveData[readIndex2];

        // Linearly interpolate within each table (lower and higher).
        float interpolationFactor = static_cast<float>(virtualReadIndex) - readIndex;
        float sampleHigher = (1 - interpolationFactor) * sample1Higher + interpolationFactor * sample2Higher;
        float sampleLower = (1 - interpolationFactor) * sample1Lower + interpolationFactor * sample2Lower;

        // Then interpolate between the two tables.
        float sample = (1 - tableInterpolationFactor) * sampleHigher + tableInterpolationFactor * sampleLower;

        *destP++ = sample;

        // Increment virtual read index and wrap virtualReadIndex into the range 0 -> waveTableSize.
        virtualReadIndex += incr;
        virtualReadIndex -= floor(virtualReadIndex * invWaveTableSize) * waveTableSize;
    }

    m_virtualReadIndex = virtualReadIndex;

    outputBus->clearSilentFlag();
}

void OscillatorNode::reset(ContextRenderLock &)
{
    m_virtualReadIndex = 0;
}

void OscillatorNode::setWaveTable(std::shared_ptr<WaveTable> waveTable)
{
    m_waveTable = waveTable;
    m_type->setUint32(static_cast<uint32_t>(OscillatorType::CUSTOM), false);  // set the value, but don't notify as that would recurse
}

bool OscillatorNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}

            *destP++ = sample;

            // Increment virtual read index and wrap virtualReadIndex into the range 0 -> waveTableSize.
            virtualReadIndex += incr;
            virtualReadIndex -= floor(virtualReadIndex * invWaveTableSize) * waveTableSize;
        }

        m_virtualReadIndex = virtualReadIndex;

        outputBus->clearSilentFlag();
    }

    void OscillatorNode::reset(ContextRenderLock &)
    {
        process_oscillator(r, bufferSize, _scheduler._renderOffset, _scheduler._renderLength);
        return;
    }

    return;
}

bool OscillatorNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}
