// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/TextureRecorderNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/BiquadFilterNode.h"
#include "LabSound/core/DynamicsCompressorNode.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"

#include <algorithm>
#include <cmath>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

using namespace lab;

namespace lab
{

AudioNodeDescriptor * TextureRecorderNode::desc()
{
    static AudioNodeDescriptor d { nullptr, nullptr, 1 };
    return &d;
}

TextureRecorderNode::TextureRecorderNode(AudioContext & ac, int width, int height, bool mirror)
    : AudioNode(ac, *desc())
    , m_width(width)
    , m_height(height)
    , m_mirror(mirror)
{
    m_sampleRate = ac.sampleRate();

    // three inputs, interpreted as R (low), G (band), B (high)
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));

    initialize();
}

TextureRecorderNode::~TextureRecorderNode()
{
    uninitialize();
}

void TextureRecorderNode::setSize(int w, int h)
{
    m_width = w;
    m_height = h;
}

void TextureRecorderNode::setMirror(bool mirror)
{
    m_mirror = mirror;
}

void TextureRecorderNode::setOverlap(float overlap)
{
    m_overlap = std::max(1.0f, overlap);
}

void TextureRecorderNode::setColorGamma(float gamma)
{
    m_gamma = std::max(0.1f, gamma);
}

// append the first channel of one input to its stream; guards unconnected inputs
static void accumulateInput(ContextRenderLock & r, AudioNode * node, int inputIndex,
                            int bufferSize, std::vector<float> & stream)
{
    auto in = node->input(inputIndex);
    if (!in || !in->isConnected())
        return;

    AudioBus * bus = in->bus(r);
    if (!bus || bus->numberOfChannels() == 0)
        return;

    const float * src = bus->channel(0)->data();
    if (!src)
        return;

    if (stream.capacity() == 0)
        stream.reserve(1024 * 1024);

    for (int i = 0; i < bufferSize; ++i)
        stream.push_back(src[i]);
}

void TextureRecorderNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * outputBus = output(0)->bus(r);

    if (m_recording)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        accumulateInput(r, this, 0, bufferSize, m_streamR);
        accumulateInput(r, this, 1, bufferSize, m_streamG);
        accumulateInput(r, this, 2, bufferSize, m_streamB);
    }

    // output(0): sum the three inputs to mono, so the node remains pullable
    // (and optionally monitorable) when connected to a destination.
    if (!outputBus)
        return;

    outputBus->zero();
    AudioChannel * outChannel = outputBus->channel(0);
    float * dst = outChannel->mutableData();
    const int outLen = outputBus->length();

    for (int idx = 0; idx < 3; ++idx)
    {
        auto in = input(idx);
        if (!in || !in->isConnected())
            continue;

        AudioBus * inBus = in->bus(r);
        if (!inBus || inBus->numberOfChannels() == 0)
            continue;

        const float * src = inBus->channel(0)->data();
        if (!src)
            continue;

        const int n = std::min(outLen, inBus->length());
        for (int i = 0; i < n; ++i)
            dst[i] += src[i];
    }
}

float TextureRecorderNode::recordedLengthInSeconds() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    size_t numSamples = m_streamR.size();
    numSamples = std::min(numSamples, m_streamG.size());
    numSamples = std::min(numSamples, m_streamB.size());
    if (m_sampleRate <= 0.f)
        return 0.f;
    return static_cast<float>(numSamples) / m_sampleRate;
}

// Shadow-lifting tone curve: brightens dim values while staying near-linear at
// mid/high values (and exactly linear at gamma == 1). Blends a gamma curve toward
// identity weighted by the value itself, so the lift concentrates in the shadows.
static inline float toneLift(float v, float gamma)
{
    if (v <= 0.f) return 0.f;
    if (gamma == 1.f) return v;
    const float g = std::pow(v, 1.f / gamma);
    return g * (1.f - v) + v * v;
}

// linear (0..1) -> 8-bit sRGB
static inline unsigned char srgb8(float v)
{
    if (v < 0.f) v = 0.f;
    if (v > 1.f) v = 1.f;
    const float s = (v <= 0.0031308f) ? (12.92f * v)
                                      : (1.055f * std::pow(v, 1.f / 2.4f) - 0.055f);
    int i = static_cast<int>(s * 255.f + 0.5f);
    if (i < 0) i = 0;
    if (i > 255) i = 255;
    return static_cast<unsigned char>(i);
}

bool TextureRecorderNode::writePNG(const std::string & filename)
{
    if (m_width <= 0 || m_height <= 0)
        return false;

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    size_t numSamples = m_streamR.size();
    numSamples = std::min(numSamples, m_streamG.size());
    numSamples = std::min(numSamples, m_streamB.size());
    if (numSamples == 0)
        return false;

    const int width = m_width;
    const int height = m_height;
    const double hop = static_cast<double>(numSamples) / static_cast<double>(width);
    const float overlap = std::max(1.0f, m_overlap);

    // RGB, black background
    std::vector<unsigned char> buf(static_cast<size_t>(width) * height * 3, 0);

    const int center = height / 2;

    // paints [y0, y1) of column x with the given color
    auto paint = [&](int x, int y0, int y1, unsigned char cr, unsigned char cg, unsigned char cb) {
        y0 = std::max(0, y0);
        y1 = std::min(height, y1);
        for (int y = y0; y < y1; ++y)
        {
            unsigned char * px = &buf[(static_cast<size_t>(y) * width + x) * 3];
            px[0] = cr;
            px[1] = cg;
            px[2] = cb;
        }
    };

    for (int x = 0; x < width; ++x)
    {
        // Peak window: this column's own time slice, for sharp transient tips.
        size_t pBegin = static_cast<size_t>(x * hop);
        size_t pEnd = static_cast<size_t>((x + 1) * hop);
        if (pEnd <= pBegin) pEnd = pBegin + 1;
        pEnd = std::min(pEnd, numSamples);
        if (pBegin >= numSamples) break;

        // RMS window: centered on the column and widened by `overlap`, so the
        // energy envelope (bar body) is smoothed across neighbouring columns.
        const double colCenter = (x + 0.5) * hop;
        const double halfWin = 0.5 * hop * overlap;
        long rBegin = static_cast<long>(colCenter - halfWin);
        long rEnd = static_cast<long>(colCenter + halfWin);
        if (rBegin < 0) rBegin = 0;
        if (rEnd > static_cast<long>(numSamples)) rEnd = static_cast<long>(numSamples);
        if (rEnd <= rBegin) rEnd = rBegin + 1;

        // RMS energy per band over the (overlapping) window
        double sumR = 0.0, sumG = 0.0, sumB = 0.0;
        for (long i = rBegin; i < rEnd; ++i)
        {
            sumR += static_cast<double>(m_streamR[i]) * m_streamR[i];
            sumG += static_cast<double>(m_streamG[i]) * m_streamG[i];
            sumB += static_cast<double>(m_streamB[i]) * m_streamB[i];
        }
        const double rCount = static_cast<double>(rEnd - rBegin);
        float rmsR = std::min(1.f, static_cast<float>(std::sqrt(sumR / rCount)));
        float rmsG = std::min(1.f, static_cast<float>(std::sqrt(sumG / rCount)));
        float rmsB = std::min(1.f, static_cast<float>(std::sqrt(sumB / rCount)));

        // Peak per band over the tight (per-slice) window
        float peakR = 0.f, peakG = 0.f, peakB = 0.f;
        for (size_t i = pBegin; i < pEnd; ++i)
        {
            peakR = std::max(peakR, std::fabs(m_streamR[i]));
            peakG = std::max(peakG, std::fabs(m_streamG[i]));
            peakB = std::max(peakB, std::fabs(m_streamB[i]));
        }
        peakR = std::min(1.f, peakR);
        peakG = std::min(1.f, peakG);
        peakB = std::min(1.f, peakB);

        const float rmsMag = std::max(rmsR, std::max(rmsG, rmsB));
        float peakMag = std::max(peakR, std::max(peakG, peakB));
        peakMag = std::max(peakMag, rmsMag);  // tip never shorter than the body

        // Bar height stays linear so event magnitudes / peak extraction are faithful.
        const int barRms = static_cast<int>(std::lround(rmsMag * height));
        const int barPeak = static_cast<int>(std::lround(peakMag * height));

        // Colors get the shadow-lift so quiet detail (breath, sibilants) is visible.
        const float gamma = m_gamma;
        const unsigned char br = srgb8(toneLift(rmsR, gamma)), bg = srgb8(toneLift(rmsG, gamma)), bb = srgb8(toneLift(rmsB, gamma));   // body
        const unsigned char tr = srgb8(toneLift(peakR, gamma)), tg = srgb8(toneLift(peakG, gamma)), tb = srgb8(toneLift(peakB, gamma)); // tip

        if (m_mirror)
        {
            const int halfRms = barRms / 2;
            const int halfPeak = barPeak / 2;
            paint(x, center - halfPeak, center + halfPeak, tr, tg, tb);  // tip first
            paint(x, center - halfRms, center + halfRms, br, bg, bb);    // body over
        }
        else
        {
            paint(x, height - barPeak, height, tr, tg, tb);  // tip first
            paint(x, height - barRms, height, br, bg, bb);   // body over
        }
    }

    return stbi_write_png(filename.c_str(), width, height, 3, buf.data(), width * 3) != 0;
}

void TextureRecorderNode::reset(ContextRenderLock & r)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_streamR.clear();
    m_streamG.clear();
    m_streamB.clear();
}

//--------------------------------------------------------------------------
// AudioTextureFixture
//--------------------------------------------------------------------------

AudioTextureFixture::AudioTextureFixture(AudioContext & ac, int width, int height, bool mirror)
{
    m_tex = std::make_shared<TextureRecorderNode>(ac, width, height, mirror);

    m_lowFilter = std::make_shared<BiquadFilterNode>(ac);
    m_bandFilter = std::make_shared<BiquadFilterNode>(ac);
    m_highFilter = std::make_shared<BiquadFilterNode>(ac);

    // Speech-oriented split (R = voicing/body, G = vowels/intelligibility,
    // B = consonants/transients):
    //
    //  Low  (R): LOWPASS 300 Hz  - vocal fundamental + body. Male F0 ~85-180 Hz
    //            and female F0 ~165-255 Hz both sit under this, plus low-formant
    //            energy, so voiced speech reads as a warm base for either gender.
    m_lowFilter->setType(lab::LOWPASS);
    m_lowFilter->frequency()->setValue(300.f);

    //  Band (G): BANDPASS ~1500 Hz, Q ~0.9 - the vowel-formant / intelligibility
    //            core (F1-F2 for both male and female voices, roughly 500-2500 Hz).
    //            This is the "speech presence" band; a moderate Q keeps it broad
    //            enough to track either voice without being smeared into the body.
    m_bandFilter->setType(lab::BANDPASS);
    m_bandFilter->frequency()->setValue(1500.f);
    m_bandFilter->q()->setValue(0.9f);

    //  High (B): HIGHPASS 3500 Hz - sibilants (s, sh, f) and plosive bursts
    //            (t, k, p). Isolating this transient/fricative band (rather than
    //            the old 800 Hz, which overlapped the voiced formants) lets
    //            plosives bleed into the high channel as useful blue signal.
    m_highFilter->setType(lab::HIGHPASS);
    m_highFilter->frequency()->setValue(3500.f);

    m_lowComp = std::make_shared<DynamicsCompressorNode>(ac);
    m_bandComp = std::make_shared<DynamicsCompressorNode>(ac);
    m_highComp = std::make_shared<DynamicsCompressorNode>(ac);

    // Lift the sibilants. LabSound's compressor applies an internal makeup gain
    // that grows as the threshold drops and the ratio rises, so a low threshold
    // + strong ratio on the high band raises quiet fricative/plosive energy to a
    // visible level. A fast attack lets the meter catch transient plosive bursts.
    m_highComp->threshold()->setValue(-45.f);  // dB; well below the quiet sibilant floor
    m_highComp->knee()->setValue(10.f);
    m_highComp->ratio()->setValue(6.f);
    m_highComp->attack()->setValue(0.002f);    // seconds; snappy onset
    m_highComp->release()->setValue(0.08f);

    // filter -> compressor
    ac.connect(m_lowComp, m_lowFilter, 0, 0);
    ac.connect(m_bandComp, m_bandFilter, 0, 0);
    ac.connect(m_highComp, m_highFilter, 0, 0);

    // compressor -> tex input 0/1/2 (R/G/B)
    ac.connect(m_tex, m_lowComp, 0, 0);
    ac.connect(m_tex, m_bandComp, 1, 0);
    ac.connect(m_tex, m_highComp, 2, 0);

    // tex -> destination (so the graph is pulled)
    ac.connect(ac.destinationNode(), m_tex, 0, 0);
}

void AudioTextureFixture::connectInput(AudioContext & ac, std::shared_ptr<AudioNode> source)
{
    ac.connect(m_lowFilter, source, 0, 0);
    ac.connect(m_bandFilter, source, 0, 0);
    ac.connect(m_highFilter, source, 0, 0);
}

}  // namespace lab
