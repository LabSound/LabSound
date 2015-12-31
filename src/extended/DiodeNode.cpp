// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/extended/DiodeNode.h"
#include <WTF/MathExtras.h>
#include <mutex>

namespace lab {
    
    // Based on the DiodeNode found at the BBC Radiophonic Workshop
    // http://webaudio.prototyping.bbc.co.uk/ring-modulator/
    DiodeNode::DiodeNode(ContextRenderLock& r, float sampleRate) : vb(0.2f), vl(0.4f), h(1.0)
    {
        waveShaper = std::make_shared<lab::WaveShaperNode>(sampleRate);
        setCurve(r);
        //initialize(); DiodeNode is not subclassed from node
    }

    void DiodeNode::setDistortion(ContextRenderLock& r, float distortion)
    {
        // We increase the distortion by increasing the gradient of the linear portion of the waveshaper's curve.
        h = distortion;
        setCurve(r);
    }

    void DiodeNode::setCurve(ContextRenderLock& r)
    {
        // The non-linear waveshaper curve describes the transformation between an input signal and an output signal.
        // We calculate a 1024-point curve following equation (2) from Parker's paper.

        int samples = 1024;
        std::shared_ptr<std::vector<float>> wsCurvePtr(new std::vector<float>());
        std::vector<float>& wsCurve = *wsCurvePtr;
        wsCurve.resize(samples);

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
            wsCurve[i] = v;
        }

        waveShaper->setCurve(r, wsCurvePtr);
    }


}
