
#ifndef LabSound_PeakCompNode_h
#define LabSound_PeakCompNode_h

// PeakComp ported to LabSound from https://github.com/twobigears/tb_peakcomp
// The copyright on the original is reproduced here in its entirety.

/*
 The MIT License (MIT)

 Copyright (c) 2013 Two Big Ears Ltd.
 http://twobigears.com

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 Stereo L+R peak compressor with variable knee-smooth, attack, release and makeup gain.
 Varun Nair. varun@twobigears.com

 Inspired by:
 http://www.eecs.qmul.ac.uk/~josh/documents/GiannoulisMassbergReiss-dynamicrangecompression-JAES2012.pdf
 https://ccrma.stanford.edu/~jos/filters/Nonlinear_Filter_Example_Dynamic.htm
 */

#include "AudioContext.h"
#include "AudioBus.h"
#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"
#include "AudioBasicProcessorNode.h"
#include "AudioBasicInspectorNode.h"
#include "GainNode.h"

namespace LabSound
{
    using namespace WebCore;

    class PeakCompNode : public WebCore::AudioBasicProcessorNode
    {
        class PeakCompNodeInternal;
        PeakCompNodeInternal * internalNode; // We do not own this!
        
    public:
        PeakCompNode(float sampleRate);
        virtual ~PeakCompNode();

		void set(float aT, float aL, float d, float s, float r);

        std::shared_ptr<AudioParam> threshold() const;
        std::shared_ptr<AudioParam> ratio() const;
        std::shared_ptr<AudioParam> attack() const;
        std::shared_ptr<AudioParam> release() const;
        std::shared_ptr<AudioParam> makeup() const;
        std::shared_ptr<AudioParam> knee() const;

    };
}
#endif
