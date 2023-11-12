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
#include "LabSound/core/PeriodicWave.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"
#include "LabSound/extended/VectorMath.h"

#include <algorithm>
//#include <emmintrin.h>

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

static char const * const s_types[] = {
    "None", "Sine", "FastSine", "Square", "Sawtooth", "Falling Sawtooth",
    "Triangle", "Custom", nullptr};

static AudioParamDescriptor s_opDesc[] = {
    {"frequency", "FREQ", 440.0,        0.0, 100000.0},
    {"detune",    "DTUN",   0.0,    -4800.0,   4800.0},
    {"amplitude", "AMPL",   1.0,        0.0, 100000.0},
    {"bias",      "BIAS",   0.0, -1000000.0, 100000.0},
    nullptr};

static AudioSettingDescriptor s_osDesc[] = {
        {"type", "TYPE", SettingType::Enum, s_types}, nullptr};

AudioNodeDescriptor * OscillatorNode::desc()
{
    static AudioNodeDescriptor d {s_opDesc, s_osDesc, 1};
    return &d;
}

OscillatorNode::OscillatorNode(AudioContext & ac)
: AudioScheduledSourceNode(ac, *desc())
, m_phaseIncrements(AudioNode::ProcessingSizeInFrames)
, m_detuneValues(AudioNode::ProcessingSizeInFrames)
{
    m_frequency = param("frequency");
    m_detune = param("detune");
    m_amplitude = param("amplitude");
    m_bias = param("bias");

    m_type = setting("type");
    m_type->setValueChanged([this]() { setType(OscillatorType(m_type->valueUint32())); });

    setType(OscillatorType::SINE);
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

void OscillatorNode::process_oscillator(ContextRenderLock & r, int bufferSize, int offset, int count)
{
    AudioBus * outputBus = output(0)->bus(r);
    if (!r.context() || !isInitialized() || !outputBus->numberOfChannels())
    {
        outputBus->zero();
        return;
    }
    
    const float sample_rate = r.context()->sampleRate();
    
    int quantumFrameOffset = offset;
    int nonSilentFramesToProcess = count;
    
    if (!nonSilentFramesToProcess)
    {
        outputBus->zero();
        return;
    }
    
    nonSilentFramesToProcess += quantumFrameOffset;
    
    if (bufferSize > m_phaseIncrements.size())
        m_phaseIncrements.allocate(bufferSize);
    if (bufferSize > m_detuneValues.size())
        m_detuneValues.allocate(bufferSize);
    if (bufferSize > m_amplitudeValues.size())
        m_amplitudeValues.allocate(bufferSize);
    if (bufferSize > m_biasValues.size())
        m_biasValues.allocate(bufferSize);
    
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
    
    if (m_detune->hasSampleAccurateValues())
    {
        // Get the sample-accurate detune values.
        float* detuneValues = m_detuneValues.data();
        float* offset_detunes = detuneValues + +quantumFrameOffset;
        m_detune->calculateSampleAccurateValues(r, offset_detunes, nonSilentFramesToProcess);
        
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
            for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
                phaseIncrements[i] *= detuneScale;
        }
    }
    
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
    const float pi = static_cast<float>(LAB_PI);
    
    OscillatorType type = static_cast<OscillatorType>(m_type->valueUint32());
    switch (type)
    {
        case OscillatorType::SINE:
            for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
            {
                destP[i] = static_cast<float>(bias[i] + amplitudes[i] * static_cast<float>(sin(phase)));
                phase += phaseIncrements[i];
                if (phase > 2.f * pi)
                    phase -= 2.f * pi;
            }
            break;
            
        case OscillatorType::FAST_SINE:
            for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
            {
                destP[i] = burk_fast_sine(phase);
                phase += phaseIncrements[i];
                if (phase > pi)
                    phase -= 0.5f * pi;
            }
            break;
            
        case OscillatorType::SQUARE:
            for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
            {
                float amp = amplitudes[i];
                destP[i] = static_cast<float>(bias[i] + (phase < pi ? amp : -amp));
                phase += phaseIncrements[i];
                if (phase > 2.f * pi)
                    phase -= 2.f * pi;
            }
            break;
            
        case OscillatorType::SAWTOOTH:
            for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
            {
                float amp = amplitudes[i];
                destP[i] = static_cast<float>(bias[i] + amp - (amp / pi * phase));
                phase += phaseIncrements[i];
                if (phase > 2.f * pi)
                    phase -= 2.f * pi;
            }
            break;
            
        case OscillatorType::FALLING_SAWTOOTH:
            for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
            {
                float amp = amplitudes[i];
                destP[i] = static_cast<float>(bias[i] - (amp / pi * phase));
                phase += phaseIncrements[i];
                if (phase > 2. * pi)
                    phase -= 2. * pi;
            }
            break;
            
        case OscillatorType::TRIANGLE:
            for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
            {
                float amp = amplitudes[i];
                if (phase < pi)
                    destP[i] = static_cast<float>(bias[i] - amp + (2.f * amp / pi * phase));
                else
                    destP[i] = static_cast<float>(bias[i] + 3.f * amp - (2.f * amp / pi *  phase));
                
                phase += phaseIncrements[i];
                if (phase > 2.f * pi)
                    phase -= 2.f * pi;
            }
            break;
            
        default: break; // other types do nothing
    }
    
    outputBus->clearSilentFlag();
}

void OscillatorNode::process(ContextRenderLock & r, int bufferSize)
{
    OscillatorType type = static_cast<OscillatorType>(m_type->valueUint32());
    if (type != OscillatorType::CUSTOM)
    {
        process_oscillator(r, bufferSize, _self->_scheduler._renderOffset, _self->_scheduler._renderLength);
        return;
    }

    return;
}

bool OscillatorNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}
