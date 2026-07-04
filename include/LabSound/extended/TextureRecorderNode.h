// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef TEXTURE_RECORDER_NODE_H
#define TEXTURE_RECORDER_NODE_H

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNode.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace lab
{

class BiquadFilterNode;
class DynamicsCompressorNode;

// TextureRecorderNode accumulates audio from three inputs (interpreted as the
// R, G, and B channels of an image) and renders a time-vs-magnitude PNG on
// demand. It follows the RecorderNode pattern: audio is captured in process()
// and a file is emitted later via writePNG().
//
// Each column x of the output image corresponds to a time slice. Each column is
// drawn as a two-tone bar: a solid "body" whose height is the overall RMS energy
// of the three bands (smoothed over an overlapping window), capped by a brighter
// "peak" tip that rises to the per-slice peak magnitude. Body color is
// sRGB(low, band, high) from the per-band RMS levels; tip color is sRGB of the
// per-band peaks, so transients (e.g. plosives) that light up the high band show
// as bright tips even when their RMS energy is low. This keeps the smooth energy
// envelope readable while preserving sharp, machine-samplable event peaks. When
// mirror is set the bar is drawn symmetrically about the vertical center for a
// classic waveform "pulse" look. The background is black.
class TextureRecorderNode : public AudioNode
{
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    bool m_recording{false};

    // one stream per input: 0 = R (low), 1 = G (band), 2 = B (high)
    std::vector<float> m_streamR;
    std::vector<float> m_streamG;
    std::vector<float> m_streamB;
    mutable std::recursive_mutex m_mutex;

    float m_sampleRate;
    int m_width;
    int m_height;
    bool m_mirror;
    float m_overlap{2.0f};  // RMS window width as a multiple of the per-column hop
    float m_gamma{1.5f};    // shadow-lift applied to colors only (1.0 = off)

public:
    TextureRecorderNode(AudioContext & ac, int width = 1024, int height = 256, bool mirror = false);
    virtual ~TextureRecorderNode();

    static  const char* static_name() { return "TextureRecorder"; }
    virtual const char* name() const override { return static_name(); }
    static  AudioNodeDescriptor * desc();

    // AudioNode
    virtual void process(ContextRenderLock &, int bufferSize) override;
    virtual void reset(ContextRenderLock &) override;

    void startRecording() { m_recording = true; }
    void stopRecording() { m_recording = false; }

    float recordedLengthInSeconds() const;

    void setSize(int w, int h);
    void setMirror(bool mirror);

    // RMS smoothing: each column's energy (bar body) is averaged over a window
    // `overlap` times wider than its time slice, centered on the column. 1.0 = no
    // overlap; larger values smooth the envelope. Peak tips stay per-slice sharp.
    void setOverlap(float overlap);

    // Shadow-lifting tone curve applied to the bar *colors* only (bar height stays
    // linear so event magnitudes remain faithful). gamma > 1 brightens dim colors
    // while staying near-linear at mid/high values; 1.0 disables it.
    void setColorGamma(float gamma);

    // writes an 8-bit sRGB PNG; returns true on success
    bool writePNG(const std::string & filename);
};

// AudioTextureFixture is a plain (non-node) helper that owns the sub-graph
//
//   source ─┬─ BiquadFilter LOWPASS  ─ DynamicsCompressor ─→ tex input 0 (R)
//           ├─ BiquadFilter BANDPASS ─ DynamicsCompressor ─→ tex input 1 (G)
//           └─ BiquadFilter HIGHPASS ─ DynamicsCompressor ─→ tex input 2 (B)
//                                              tex ─→ destinationNode (pull)
//
// It wires everything up in the constructor; connectInput() then fans an
// external source into all three filters.
class AudioTextureFixture
{
    std::shared_ptr<TextureRecorderNode> m_tex;

    std::shared_ptr<BiquadFilterNode> m_lowFilter;
    std::shared_ptr<BiquadFilterNode> m_bandFilter;
    std::shared_ptr<BiquadFilterNode> m_highFilter;

    std::shared_ptr<DynamicsCompressorNode> m_lowComp;
    std::shared_ptr<DynamicsCompressorNode> m_bandComp;
    std::shared_ptr<DynamicsCompressorNode> m_highComp;

public:
    AudioTextureFixture(AudioContext & ac, int width, int height, bool mirror = false);
    ~AudioTextureFixture() = default;

    // fan an external source into all three band filters
    void connectInput(AudioContext & ac, std::shared_ptr<AudioNode> source);

    void startRecording() { m_tex->startRecording(); }
    void stopRecording() { m_tex->stopRecording(); }

    bool writePNG(const std::string & filename) { return m_tex->writePNG(filename); }

    std::shared_ptr<TextureRecorderNode> textureRecorder() const { return m_tex; }
    std::shared_ptr<BiquadFilterNode> lowFilter() const { return m_lowFilter; }
    std::shared_ptr<BiquadFilterNode> bandFilter() const { return m_bandFilter; }
    std::shared_ptr<BiquadFilterNode> highFilter() const { return m_highFilter; }
    std::shared_ptr<DynamicsCompressorNode> lowCompressor() const { return m_lowComp; }
    std::shared_ptr<DynamicsCompressorNode> bandCompressor() const { return m_bandComp; }
    std::shared_ptr<DynamicsCompressorNode> highCompressor() const { return m_highComp; }
};

}  // end namespace lab

#endif
