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

using namespace lab;

void OscillatorNode::process_oscillator(ContextRenderLock & r, int framesToProcess)
{
    AudioBus * outputBus = output(0)->bus(r);
    if (!r.context() || !isInitialized() || !outputBus->numberOfChannels())
    {
        outputBus->zero();
        return;
    }

    size_t quantumFrameOffset       = 0;
    size_t nonSilentFramesToProcess = 0;
    updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);

    if (!nonSilentFramesToProcess)
    {
        outputBus->zero();
        return;
    }

    if (framesToProcess > m_phaseIncrements.size()) m_phaseIncrements.allocate(framesToProcess);
    if (framesToProcess > m_detuneValues.size()) m_detuneValues.allocate(framesToProcess);
    if (framesToProcess > m_amplitudeValues.size()) m_amplitudeValues.allocate(framesToProcess);
    if (framesToProcess > m_biasValues.size()) m_biasValues.allocate(framesToProcess);

    // calculate phase increments
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
        {
            phaseIncrements[i] = frequency;
        }
    }

    if (m_detune->hasSampleAccurateValues())
    {
        // Get the sample-accurate detune values.
        float * detuneValues = m_detuneValues.data();
        m_detune->calculateSampleAccurateValues(r, detuneValues, framesToProcess);

        // Convert from cents to rate scalar and perform detuning
        float k = 1.f / 1200.f;
        VectorMath::vsmul(detuneValues, 1, &k, detuneValues, 1, framesToProcess);
        for (int i = 0; i < framesToProcess; ++i)
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
                destP[i]  = static_cast<float>(bias[i] + (_lab_phase < M_PI ? amp : -amp));
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
                destP[i]  = static_cast<float>(bias[i] + amp - (amp / M_PI * _lab_phase));
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

void OscillatorNode::process(ContextRenderLock & r, size_t framesToProcess)
{
    OscillatorType type = static_cast<OscillatorType>(m_type->valueUint32());
    if (type != OscillatorType::CUSTOM)
    {
        process_oscillator(r, static_cast<int>(framesToProcess));
        return;
    }

    return;
}

void OscillatorNode::reset(ContextRenderLock &)
{
}

bool OscillatorNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}
