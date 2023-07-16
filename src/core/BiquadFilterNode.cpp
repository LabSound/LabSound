// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/BiquadFilterNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Registry.h"

#include <algorithm>

//---- begin biquad.h
/* Simple implementation of Biquad filters -- Tom St Denis
 *
 * Based on the work

Cookbook formulae for audio EQ biquad filter coefficients
---------------------------------------------------------
by Robert Bristow-Johnson, pbjrbj@viconet.com  a.k.a. robert@audioheads.com

 * Available on the web at

http://www.smartelectronix.com/musicdsp/text/filters005.txt

 * Enjoy.
 *
 * This work is hereby placed in the public domain for all purposes, whether
 * commercial, free [as in speech] or educational, etc.  Use the code and please
 * give me credit if you wish.
 *
 * Tom St Denis -- http://tomstdenis.home.dhs.org
*/

/* this would be biquad.h */


/* whatever sample type you want */
typedef float smp_type;

/* this holds the data required to update samples thru a filter */
typedef struct {
    int type;
    smp_type dbGain;
    smp_type freq;
    smp_type bandwidth;
    /* assuming srate to be constant */
    smp_type a0, a1, a2, a3, a4; /* cached for update */
    smp_type x1, x2, y1, y2;
}
biquad;

// @todo line these up with the audio node's

/* filter types */
enum {
    LPF, /* low pass filter */
    HPF, /* High pass filter */
    BPF, /* band pass filter */
    NOTCH, /* Notch Filter */
    PEQ, /* Peaking band EQ filter */
    LSH, /* Low shelf filter */
    HSH /* High shelf filter */
};


/* Below this would be biquad.c */
/* Computes a BiQuad filter on a sample */
static smp_type BiQuad(smp_type sample, biquad * b)
{
    smp_type result;

    /* compute result */
    result = b->a0 * sample + b->a1 * b->x1 + b->a2 * b->x2 -
        b->a3 * b->y1 - b->a4 * b->y2;

    /* shift x1 to x2, sample to x1 */
    b->x2 = b->x1;
    b->x1 = sample;

    /* shift y1 to y2, result to y1 */
    b->y2 = b->y1;
    b->y1 = result;

    return result;
}


static void BiQuad_update(biquad *b,
                             int type,
                             smp_type dbGain,
                             smp_type freq,
                             smp_type srate,
                             smp_type bandwidth)
{
    smp_type A, omega, sn, cs, alpha, beta;
    smp_type a0, a1, a2, b0, b1, b2;

    b->type = type;
    b->dbGain = dbGain;
    b->freq = freq;
    b->bandwidth = bandwidth;

    /* setup variables */
    A = pow(10, dbGain /40);
    omega = 2 * M_PI * freq /srate;
    sn = sin(omega);
    cs = cos(omega);
    alpha = sn * sinh(M_LN2 /2 * bandwidth * omega /sn);
    beta = sqrt(A + A);

    switch (type) {
    case LPF:
        b0 = (1 - cs) /2;
        b1 = 1 - cs;
        b2 = (1 - cs) /2;
        a0 = 1 + alpha;
        a1 = -2 * cs;
        a2 = 1 - alpha;
        break;
    case HPF:
        b0 = (1 + cs) /2;
        b1 = -(1 + cs);
        b2 = (1 + cs) /2;
        a0 = 1 + alpha;
        a1 = -2 * cs;
        a2 = 1 - alpha;
        break;
    case BPF:
        b0 = alpha;
        b1 = 0;
        b2 = -alpha;
        a0 = 1 + alpha;
        a1 = -2 * cs;
        a2 = 1 - alpha;
        break;
    case NOTCH:
        b0 = 1;
        b1 = -2 * cs;
        b2 = 1;
        a0 = 1 + alpha;
        a1 = -2 * cs;
        a2 = 1 - alpha;
        break;
    case PEQ:
        b0 = 1 + (alpha * A);
        b1 = -2 * cs;
        b2 = 1 - (alpha * A);
        a0 = 1 + (alpha /A);
        a1 = -2 * cs;
        a2 = 1 - (alpha /A);
        break;
    case LSH:
        b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
        b1 = 2 * A * ((A - 1) - (A + 1) * cs);
        b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
        a0 = (A + 1) + (A - 1) * cs + beta * sn;
        a1 = -2 * ((A - 1) + (A + 1) * cs);
        a2 = (A + 1) + (A - 1) * cs - beta * sn;
        break;
    case HSH:
        b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
        b1 = -2 * A * ((A - 1) + (A + 1) * cs);
        b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
        a0 = (A + 1) - (A - 1) * cs + beta * sn;
        a1 = 2 * ((A - 1) - (A + 1) * cs);
        a2 = (A + 1) - (A - 1) * cs - beta * sn;
        break;
    default:
        break;
    }

    /* precompute the coefficients */
    b->a0 = b0 /a0;
    b->a1 = b1 /a0;
    b->a2 = b2 /a0;
    b->a3 = a1 /a0;
    b->a4 = a2 /a0;
}

/* sets up a BiQuad Filter */
static bool BiQuad_init(biquad* b, int type, smp_type dbGain, smp_type freq,
smp_type srate, smp_type bandwidth)
{
    if (b == NULL)
        return false;
    BiQuad_update (b, type, dbGain, freq, srate, bandwidth);

    /* zero initial samples */
    b->x1 = b->x2 = 0;
    b->y1 = b->y2 = 0;

    return true;
}

// end biquad.h

namespace lab
{

static char const * const s_filter_types[FilterType::_FilterTypeCount + 1] = {
    "None",
    "Low Pass", "High Pass", "Band Pass", "Low Shelf", "High Shelf", "Peaking", "Notch", "All Pass",
    nullptr};

static AudioParamDescriptor s_bqParams[] = {
    {"frequency", "FREQ", 350.0, 10.0, 22500.0},
    {"Q", "Q   ", 1.0, 0.0001, 1000.0},
    {"gain", "GAIN", 0.0, -40.0, 40.0},
    {"detune", "DTUN", 0.0, -4800.0, 4800.0},
    nullptr};

static AudioSettingDescriptor s_bqSettings[] = {
    {"type", "TYPE", SettingType::Enum, s_filter_types},
    nullptr};

AudioNodeDescriptor* BiquadFilterNode::desc() {
    static AudioNodeDescriptor d {s_bqParams, s_bqSettings, 0};
    return &d;
}

class BiquadFilterNode::BiquadFilterNodeInternal : public AudioProcessor
{
    friend class BiquadFilterNode;
    biquad the_filter;
    bool must_reset;
    
public:

    BiquadFilterNodeInternal(BiquadFilterNode* self)
    : AudioProcessor()
    , must_reset(true)
    {
        m_frequency = self->param("frequency");
        m_q = self->param("Q");
        m_gain = self->param("gain");
        m_detune = self->param("detune");
        m_type = self->setting("type");
        m_type->setEnumeration(static_cast<uint32_t>(FilterType::LOWPASS));
        m_type->setValueChanged([this]() {
            uint32_t type = this->m_type->valueUint32();
            if (type > static_cast<uint32_t>(FilterType::ALLPASS))
                throw std::out_of_range("Filter type exceeds index of known types");
            this->must_reset = true;  // reset filter coeffs
        });
    }

    virtual ~BiquadFilterNodeInternal() = default;

    void initialize() override {}
    void uninitialize() override {}

    virtual void process(ContextRenderLock & r,
                         const lab::AudioBus * sourceBus,
                         lab::AudioBus * destinationBus,
                         int framesToProcess) override
    {
        const float* frequency = m_frequency->bus()->channel(0)->data();
        const float* gain = m_gain->bus()->channel(0)->data();
        const float *q = m_q->bus()->channel(0)->data();
        
        uint32_t type = this->m_type->valueUint32();
        
        auto ac = r.context();

        if (must_reset) {
            BiQuad_init(&the_filter, type, gain[0], frequency[0], ac->sampleRate(), q[0]);
            must_reset = false;
        }
        
        const float* in = sourceBus->channel(0)->data();
        float* out = destinationBus->channel(0)->mutableData();
        
        for (int i = 0; i < framesToProcess; ++i) {
            BiQuad_update(&the_filter, type, gain[0],
                          frequency[0], ac->sampleRate(), q[0]);
            out[i] = BiQuad(in[i], &the_filter);
        }
    }

    virtual double tailTime(ContextRenderLock & r) const override { return 0.25f; } // fixed 250ms
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }


    void getFrequencyResponse(ContextRenderLock & r, const std::vector<float> & frequencyHz, std::vector<float> & magResponse, std::vector<float> & phaseResponse)
    {
        if (!frequencyHz.size() || !magResponse.size() || !phaseResponse.size())
            return;

        size_t n = std::min(frequencyHz.size(), std::min(magResponse.size(), phaseResponse.size()));

        // @todo
        if (n)
        {
            // std::unique_ptr<BiquadDSPKernel> responseKernel(new BiquadDSPKernel(this));
            // responseKernel->getFrequencyResponse(r, nFrequencies, frequencyHz, magResponse, phaseResponse);
            //biquadProcessor()->getFrequencyResponse(r, n, &frequencyHz[0], &magResponse[0], &phaseResponse[0]);
        }
    }
    
    void reset() override {
        must_reset = true;
    }

    std::shared_ptr<AudioSetting> m_type;
    std::shared_ptr<AudioParam> m_frequency;
    std::shared_ptr<AudioParam> m_q;
    std::shared_ptr<AudioParam> m_gain;
    std::shared_ptr<AudioParam> m_detune;
};
 
BiquadFilterNode::BiquadFilterNode(AudioContext & ac)
: AudioBasicProcessorNode(ac, *desc())
{
    biquad_impl = new BiquadFilterNodeInternal(this);
    m_processor.reset(biquad_impl);
    initialize();
}

BiquadFilterNode::~BiquadFilterNode()
{
    if (isInitialized()) uninitialize();
}

void BiquadFilterNode::setType(FilterType type)
{
    biquad_impl->m_type->setUint32(uint32_t(type));
}

void BiquadFilterNode::getFrequencyResponse(ContextRenderLock & r, const std::vector<float> & frequencyHz, std::vector<float> & magResponse, std::vector<float> & phaseResponse)
{
    biquad_impl->getFrequencyResponse(r, frequencyHz, magResponse, phaseResponse);
}

FilterType BiquadFilterNode::type() const
{
    return (FilterType) biquad_impl->m_type->valueUint32();
}

std::shared_ptr<AudioParam> BiquadFilterNode::frequency()
{
    return biquad_impl->m_frequency;
}

std::shared_ptr<AudioParam> BiquadFilterNode::q()
{
    return biquad_impl->m_q;
}

std::shared_ptr<AudioParam> BiquadFilterNode::gain()
{
    return biquad_impl->m_gain;
}

std::shared_ptr<AudioParam> BiquadFilterNode::detune()
{
    return biquad_impl->m_detune;
}

}  // namespace lab
