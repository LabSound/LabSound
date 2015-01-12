
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

#include "PeakCompNode.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "LabSound.h"
#include "AudioProcessor.h"
#include "VectorMath.h"

#include <vector>

using namespace WebCore;
using namespace WebCore::VectorMath;
using namespace LabSound;
using namespace std;

namespace LabSound {


    class PeakCompNode::PeakCompNodeInternal : public WebCore::AudioProcessor {
    public:

        PeakCompNodeInternal(float sampleRate)
        : AudioProcessor(sampleRate)
        , numChannels(1)
        {
            m_threshold = std::make_shared<AudioParam>("threshold",  0, 0, -1e6f);   // db
            m_ratio = std::make_shared<AudioParam>("ratio",  1, 0, 10);   // default 1:1
            m_attack = std::make_shared<AudioParam>("attack",   0.001f,  0, 1000);   // attack in ms
            m_release = std::make_shared<AudioParam>("release", 0.001f, 0, 1000);   // release in ms
            m_makeup = std::make_shared<AudioParam>("makeup", 0, 0, 60);   // makeup gain, in db
            m_knee = std::make_shared<AudioParam>("knee", 0, 0, 1);   // knee smoothing, 0 = hard, 1 = smooth. Default is 0

            // Initialise parameters

            for (int i = 0; i < 2; i++) {
                kneeRecursive[i] = 0.f;
                attackRecursive[i] = 0.f;
                releaseRecursive[i] = 0.f;
            }

            // Get sample rate
            fFs = sampleRate;
            onebyfFS = 1.0f / fFs;
        }

        virtual ~PeakCompNodeInternal() {
        }

        // AudioProcessor interface
        virtual void initialize() {
        }

        virtual void uninitialize() { }

        // Processes the source to destination bus.  The number of channels must match in source and destination.
        virtual void process(ContextGraphLock& g, ContextRenderLock& r, const WebCore::AudioBus* sourceBus, WebCore::AudioBus* destinationBus, size_t framesToProcess) {
            if (!numChannels)
                return;

            // copy attributes to run time variables
            float v = m_threshold->value(r);
            if (v <= 0) {
                // dB to linear (could use the function from m_pd.h)
                threshold = powf(10, (v*0.05f));
            }
            else
                threshold = 0;

            v = m_ratio->value(r);
            if (v >= 1) {
                ratio = 1.f/v;
            }
            else
                ratio = 1;

            v = m_attack->value(r);
            if (v >= 0.001) {
                attack = v * 0.001;
            }
            else
                attack = 0.000001;

            v = m_release->value(r);
            if (v >= 0.001) {
                release = v * 0.001;
            }
            else
                release = 0.000001;

            v = m_makeup->value(r);
            // dB to linear (could use the function from m_pd.h)
            makeupGain = pow(10, (v * 0.05));

            v = m_knee->value(r);
            if (v >= 0 && v <= 1)
            {
                // knee value (0 to 1) is scaled from 0 (hard) to 0.02 (smooth). Could be scaled to a larger number.
                knee = v * 0.02;
            }

            // calc coefficients from run time vars
            kneeCoeffs = expf(0.f - (onebyfFS / knee));
            kneeCoeffsMinus = 1.f - kneeCoeffs;
            attackCoeffs = expf(0.f - (onebyfFS / attack));
            attackCoeffsMinus = 1.f - attackCoeffs;
            releaseCoeff = expf(0.f - (onebyfFS / release));
            releaseCoeffMinus = 1.f - releaseCoeff;

            // Handle both the 1 -> N and N -> N case here.
            const float* source[16];
            for (int i = 0; i < numChannels; ++i) {
                if (sourceBus->numberOfChannels() == numChannels)
                    source[i] = sourceBus->channel(i)->data();
                else
                    source[i] = sourceBus->channel(0)->data();
            }
            float* dest[16];
            for (int i = 0; i < numChannels; ++i)
                dest[i] = destinationBus->channel(i)->mutableData();

            for (int i = 0; i < framesToProcess; ++i) {
                float peakEnv = 0;
                for (int j = 0; j < numChannels; ++j) {
                    peakEnv += source[j][i];
                }
                //release recursive
                releaseRecursive[0] = (releaseCoeffMinus * peakEnv) + (releaseCoeff * std::max(peakEnv, releaseRecursive[1]));
                //attack recursive
                attackRecursive[0] = ((attackCoeffsMinus * releaseRecursive[0]) + (attackCoeffs * attackRecursive[1]));
                //knee smoothening and gain reduction
                kneeRecursive[0] = (kneeCoeffsMinus * std::max(std::min(((threshold + (ratio * (attackRecursive[0] - threshold))) / attackRecursive[0]), 1.f), 0.f)) + (kneeCoeffs * kneeRecursive[1]);

                for (int j = 0; j < numChannels; ++j) {
                    dest[j][i] = source[j][i] * kneeRecursive[0] * makeupGain;
                }
            }
            releaseRecursive[1] = releaseRecursive[0];
            attackRecursive[1] = attackRecursive[0];
            kneeRecursive[1] = kneeRecursive[0];
        }


        double		f_float2Sig;		// Dummy variable for conversion of floats in inlet to signals
        float		fFs;				// Sample rate

        // Arrays for delay lines
        float 		kneeRecursive[2];
        float 		attackRecursive[2];
        float 		releaseRecursive[2];

        // Values
        float 		attack;
        float 		release;
        float 		ratio;
        float 		threshold;
        float 		knee;
        float 		kneeCoeffs;
        float 		kneeCoeffsMinus;
        float 		attackCoeffs;
        float 		attackCoeffsMinus;
        float 		releaseCoeff;
        float 		releaseCoeffMinus;
        float 		onebyfFS;           // one over sample rate
        float 		makeupGain;

        // Resets filter state
        virtual void reset() { }

        virtual void setNumberOfChannels(unsigned i) {
            if (i > 16)
                numChannels = 16;
            else
                numChannels = i;
        }

        virtual double tailTime() const { return 0; }
        virtual double latencyTime() const { return 0; }
        
        int numChannels;

		std::shared_ptr<AudioParam> m_threshold;
		std::shared_ptr<AudioParam> m_ratio;
		std::shared_ptr<AudioParam> m_attack;
		std::shared_ptr<AudioParam> m_release;
		std::shared_ptr<AudioParam> m_makeup;
		std::shared_ptr<AudioParam> m_knee;
    };

    std::shared_ptr<AudioParam> PeakCompNode::threshold() const { return data->m_threshold; }
    std::shared_ptr<AudioParam> PeakCompNode::ratio() const { return data->m_ratio; }
    std::shared_ptr<AudioParam> PeakCompNode::attack() const { return data->m_attack; }
    std::shared_ptr<AudioParam> PeakCompNode::release() const { return data->m_release; }
    std::shared_ptr<AudioParam> PeakCompNode::makeup() const { return data->m_makeup; }
    std::shared_ptr<AudioParam> PeakCompNode::knee() const { return data->m_knee; }

    PeakCompNode::PeakCompNode(float sampleRate)
    : WebCore::AudioBasicProcessorNode(sampleRate)
    , data(new PeakCompNodeInternal(sampleRate))
    {
        m_processor = std::move(std::unique_ptr<WebCore::AudioProcessor>(data));

        setNodeType((AudioNode::NodeType) LabSound::NodeTypePeakComp);

        addInput(std::unique_ptr<AudioNodeInput>(new WebCore::AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new WebCore::AudioNodeOutput(this, 2))); // 2 stereo
        
        initialize();
    }
    
    PeakCompNode::~PeakCompNode() {
        data->numChannels = 0;
        delete data;
        data = 0;
        uninitialize();
    }
    
} // End namespace LabSound
