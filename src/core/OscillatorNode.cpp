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

using namespace std;

namespace lab
{
using namespace VectorMath;

// https://en.wikibooks.org/wiki/Sound_Synthesis_Theory/Oscillators_and_Wavetables

void OscillatorNode::process_oscillator(ContextRenderLock & r, int framesToProcess)
{
    // housekeeping
    //
    AudioBus * outputBus = output(0)->bus(r);
    if (!r.context() || !isInitialized() || !outputBus->numberOfChannels())
    {
        outputBus->zero();
        return;
    }

    size_t quantumFrameOffset = 0;
    size_t nonSilentFramesToProcess = 0;
    updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);
    if (!nonSilentFramesToProcess)
    {
        outputBus->zero();
        return;
    }

    if (framesToProcess > m_phaseIncrements.size())
    {
        m_phaseIncrements.allocate(framesToProcess);
    }
    if (framesToProcess > m_detuneValues.size())
    {
        m_detuneValues.allocate(framesToProcess);
    }
    if (framesToProcess > m_amplitudeValues.size())
    {
        m_amplitudeValues.allocate(framesToProcess);
    }
    if (framesToProcess > m_biasValues.size())
    {
        m_biasValues.allocate(framesToProcess);
    }

    // calculate phase increments
    //
    float * phaseIncrements = m_phaseIncrements.data();

    if (m_frequency->hasSampleAccurateValues())
    {
        // Get the sample-accurate frequency values in preparation for conversion to phase increments.
        // They will be converted to phase increments below.
        m_frequency->calculateSampleAccurateValues(r, phaseIncrements, framesToProcess);
    }
    else
    {
        // Handle ordinary parameter smoothing/de-zippering if there are no scheduled changes.
        m_frequency->smooth(r);
        float frequency = m_frequency->smoothedValue();
        for (int i = 0; i < framesToProcess; ++i)
            phaseIncrements[i] = frequency;
    }

    if (m_detune->hasSampleAccurateValues())
    {
        // Get the sample-accurate detune values.
        float * detuneValues = m_detuneValues.data();
        m_detune->calculateSampleAccurateValues(r, detuneValues, framesToProcess);

        // Convert from cents to rate scalar and perform detuning
        float k = 1.f / 1200.f;
        vsmul(detuneValues, 1, &k, detuneValues, 1, framesToProcess);
        for (int i = 0; i < framesToProcess; ++i)
            phaseIncrements[i] *= powf(2, detuneValues[i]);  // FIXME: converting to expf() will be faster.
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
                phaseIncrements[i] *= detuneScale;
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
            for (int i = 0; i < framesToProcess; ++i)
            {
                destP[i] = static_cast<float>(bias[i] + amplitudes[i] * static_cast<float>(sin(_lab_phase)));
                _lab_phase += phaseIncrements[i];
            }

            if (_lab_phase > 2. * M_PI)
                _lab_phase -= 2. * M_PI;
        }
        break;

        case OscillatorType::SQUARE:
        {
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

    // Default to no detuning.
    m_detune = std::make_shared<AudioParam>("detune", "DTUN", 0, -4800, 4800);

    m_amplitude = std::make_shared<AudioParam>("amplitude", "AMPL", 1, 0, 100000);
    m_bias = std::make_shared<AudioParam>("bias", "BIAS", 0, -1000000, 100000);

    m_params.push_back(m_frequency);
    m_params.push_back(m_amplitude);
    m_params.push_back(m_bias);
    m_params.push_back(m_detune);

    m_type->setValueChanged(
        [this]() {
            _setType(OscillatorType(m_type->valueUint32()));
        });
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
    m_type->setUint32(static_cast<uint32_t>(type), false);

    if (type < OscillatorType::CUSTOM)
        return;

    /// @TODO not yet implemented
    if (type == OscillatorType::CUSTOM)
        return;

    std::shared_ptr<WaveTable> waveTable;
    switch (type)
    {
        case OscillatorType::SINE:
            if (!s_waveTableSine)
            {
                s_waveTableSine = std::make_shared<WaveTable>(m_sampleRate, OscillatorType::SINE);
            }
            waveTable = s_waveTableSine;
            break;
        case OscillatorType::SQUARE:
            if (!s_waveTableSquare)
            {
                s_waveTableSquare = std::make_shared<WaveTable>(m_sampleRate, OscillatorType::SQUARE);
            }
            waveTable = s_waveTableSquare;
            break;
        case OscillatorType::SAWTOOTH:
            if (!s_waveTableSawtooth)
            {
                s_waveTableSawtooth = std::make_shared<WaveTable>(m_sampleRate, OscillatorType::SAWTOOTH);
            }
            waveTable = s_waveTableSawtooth;
            break;
        case OscillatorType::TRIANGLE:
            if (!s_waveTableTriangle)
            {
                s_waveTableTriangle = std::make_shared<WaveTable>(m_sampleRate, OscillatorType::TRIANGLE);
            }
            waveTable = s_waveTableTriangle;
            break;
        case OscillatorType::CUSTOM:
        default:
            throw std::invalid_argument("Cannot set wavtable for custom type");
    }

    setWaveTable(waveTable);
}

bool OscillatorNode::calculateSampleAccuratePhaseIncrements(ContextRenderLock & r, size_t framesToProcess)
{
    bool isGood = framesToProcess <= m_phaseIncrements.size() && framesToProcess <= m_detuneValues.size();
    ASSERT(isGood);
    if (!isGood)
        return false;

    if (m_firstRender)
    {
        m_firstRender = false;
        m_frequency->resetSmoothedValue();
        m_detune->resetSmoothedValue();
    }

    bool hasSampleAccurateValues = false;
    bool hasFrequencyChanges = false;
    float * phaseIncrements = m_phaseIncrements.data();

    float finalScale = m_waveTable->rateScale();

    if (m_frequency->hasSampleAccurateValues())
    {
        hasSampleAccurateValues = true;
        hasFrequencyChanges = true;

        // Get the sample-accurate frequency values in preparation for conversion to phase increments.
        // They will be converted to phase increments below.
        m_frequency->calculateSampleAccurateValues(r, phaseIncrements, framesToProcess);
    }
    else
    {
        // Handle ordinary parameter smoothing/de-zippering if there are no scheduled changes.
        m_frequency->smooth(r);
        float frequency = m_frequency->smoothedValue();
        finalScale *= frequency;
    }

    if (m_detune->hasSampleAccurateValues())
    {
        hasSampleAccurateValues = true;

        // Get the sample-accurate detune values.
        float * detuneValues = hasFrequencyChanges ? m_detuneValues.data() : phaseIncrements;
        m_detune->calculateSampleAccurateValues(r, detuneValues, framesToProcess);

        // Convert from cents to rate scalar.
        float k = 1.f / 1200.f;
        vsmul(detuneValues, 1, &k, detuneValues, 1, framesToProcess);
        for (unsigned i = 0; i < framesToProcess; ++i)
            detuneValues[i] = powf(2, detuneValues[i]);  // FIXME: converting to expf() will be faster.

        if (hasFrequencyChanges)
        {
            // Multiply frequencies by detune scalings.
            vmul(detuneValues, 1, phaseIncrements, 1, phaseIncrements, 1, framesToProcess);
        }
    }
    else
    {
        // Handle ordinary parameter smoothing/de-zippering if there are no scheduled changes.
        m_detune->smooth(r);
        float detune = m_detune->smoothedValue();
        float detuneScale = powf(2, detune / 1200);
        finalScale *= detuneScale;
    }

    if (hasSampleAccurateValues)
    {
        // Convert from frequency to wavetable increment.
        //    source        stride  scalar        dest        stride   count
        vsmul(phaseIncrements, 1, &finalScale, phaseIncrements, 1, framesToProcess);
    }

    return hasSampleAccurateValues;
}

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

    AudioBus * outputBus = output(0)->bus(r);

    if (!isInitialized() || !outputBus->numberOfChannels())
    {
        outputBus->zero();
        return;
    }

    ASSERT(framesToProcess <= m_phaseIncrements.size());
    if (framesToProcess > m_phaseIncrements.size())
        return;

    // The audio thread can't block on this lock, so we call tryLock() instead.
    if (!r.context())
    {
        // Too bad - the tryLock() failed. We must be in the middle of changing wave-tables.
        outputBus->zero();
        return;
    }

    // We must access m_waveTable only inside the lock.
    if (!m_waveTable.get())
    {
        outputBus->zero();
        return;
    }

    size_t quantumFrameOffset = 0;
    size_t nonSilentFramesToProcess = 0;

    updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);

    if (!nonSilentFramesToProcess)
    {
        outputBus->zero();
        return;
    }

    unsigned waveTableSize = m_waveTable->periodicWaveSize();
    double invWaveTableSize = 1.0 / waveTableSize;

    float * destP = outputBus->channel(0)->mutableData();

    ASSERT(quantumFrameOffset <= framesToProcess);

    // We keep virtualReadIndex double-precision since we're accumulating values.
    double virtualReadIndex = m_virtualReadIndex;

    float rateScale = m_waveTable->rateScale();
    float invRateScale = 1 / rateScale;
    bool hasSampleAccurateValues = calculateSampleAccuratePhaseIncrements(r, framesToProcess);

    float frequency = 0;
    float * higherWaveData = 0;
    float * lowerWaveData = 0;
    float tableInterpolationFactor;

    if (!hasSampleAccurateValues)
    {
        frequency = m_frequency->smoothedValue();
        float detune = m_detune->smoothedValue();
        float detuneScale = powf(2, detune / 1200);
        frequency *= detuneScale;
        m_waveTable->waveDataForFundamentalFrequency(frequency, lowerWaveData, higherWaveData, tableInterpolationFactor);
    }

    float incr = frequency * rateScale;
    float * phaseIncrements = m_phaseIncrements.data();

    unsigned readIndexMask = waveTableSize - 1;

    // Start rendering at the correct offset.
    destP += quantumFrameOffset;
    size_t n = nonSilentFramesToProcess;

    while (n--)
    {
        unsigned readIndex = static_cast<unsigned>(virtualReadIndex);
        unsigned readIndex2 = readIndex + 1;

        // Contain within valid range.
        readIndex = readIndex & readIndexMask;
        readIndex2 = readIndex2 & readIndexMask;

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

}  // namespace lab
