//
//  DiodeNode.cpp
//  LabSound
//
//  Based on the DiodeNode found at the BBC Radiophonic Workshop
//  http://webaudio.prototyping.bbc.co.uk/ring-modulator/
//
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

// PdNode wraps an instance of pure-data as a signal processing node

#include "DiodeNode.h"
#include <mutex>

namespace LabSound {

    DiodeNode::DiodeNode(WebCore::AudioContext* context)
    : vb(0.2f)
    , vl(0.4f)
    , h(1.0)
    {
        waveShaper = context->createWaveShaper();
        setCurve();
    }

    void DiodeNode::setDistortion(float distortion)
    {
        // We increase the distortion by increasing the gradient of the linear portion of the waveshaper's curve.
        h = distortion;
        setCurve();
    }

    void DiodeNode::setCurve()
    {
        // The non-linear waveshaper curve describes the transformation between an input signal and an output signal.
        // We calculate a 1024-point curve following equation (2) from Parker's paper.

        int samples = 1024;
        RefPtr<Float32Array> wsCurve = Float32Array::create(samples);

        float* data = wsCurve->data();
        float s = float(samples);
        for (int i = 0; i < samples; ++i) {
            // convert the index to a voltage of range -1 to 1
            float v = fabsf((float(i) - s / 2.0f) / (s / 2.0f));
            if (v <= vb)
                v = 0;
            else if ((vb < v) && (v <= vl))
                v = h * powf(v - vb, 2.0f) / (2 * vl - 2.0f * vb);
            else
                v = h * v - h * vl + (h * ((powf(vl - vb, 2.0f)) / (2.0f * vl - 2.0f * vb)));
            data[i] = v;
        }

        waveShaper->setCurve(wsCurve.get()); // The Float32Array becomes owned by the waveShaper
    }


} // LabSound
