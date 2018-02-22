// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/NoiseNode.h"

using namespace std;
using namespace lab;

namespace lab {

    NoiseNode::NoiseNode() : AudioScheduledSourceNode()
    {
        addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

        initialize();
    }

    NoiseNode::~NoiseNode()
    {
        uninitialize();
    }

    void NoiseNode::setType(NoiseType type)
    {
        if (type > NOISE_TYPE_END) throw std::out_of_range("Noise argument exceeds known noise types");
        m_type = type;
    }


    void NoiseNode::process(ContextRenderLock& r, size_t framesToProcess)
    {
        AudioBus* outputBus = output(0)->bus(r);

        if (!isInitialized() || !outputBus->numberOfChannels()) {
            outputBus->zero();
            return;
        }

        size_t quantumFrameOffset;
        size_t nonSilentFramesToProcess;

        updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);

        if (!nonSilentFramesToProcess) {
            outputBus->zero();
            return;
        }

        float* destP = outputBus->channel(0)->mutableData();

        // Start rendering at the correct offset.
        destP += quantumFrameOffset;
        int n = nonSilentFramesToProcess;

        switch (m_type) {
                // reference: http://noisehack.com/generate-noise-web-audio-api/
            case WHITE:
                while (n--) {
                    float white = ((float)((whiteSeed & 0x7fffffff) - 0x40000000)) * (float)(1.0f / 0x40000000);
                    white = (white * 0.5f) - 1.0f;
                    *destP++ = white;
                    whiteSeed = whiteSeed * 435898247 + 382842987;
                }
                break;
            case PINK:
                while (n--) {
                    float white = ((float)((whiteSeed & 0x7fffffff) - 0x40000000)) * (float)(1.0f / 0x40000000);
                    white = (white * 0.5f) - 1.0f;
                    whiteSeed = whiteSeed * 435898247 + 382842987;

                    pink0 = 0.99886f * pink0 + white * 0.0555179f;
                    pink1 = 0.99332f * pink1 + white * 0.0750759f;
                    pink2 = 0.96900f * pink2 + white * 0.1538520f;
                    pink3 = 0.86650f * pink3 + white * 0.3104856f;
                    pink4 = 0.55000f * pink4 + white * 0.5329522f;
                    pink5 = -0.7616f * pink5 - white * 0.0168980f;
                    *destP++ = (pink0 + pink1 + pink2 + pink3 + pink4 + pink5 + pink6 + (white * 0.5362f)) * 0.11f; // .11 roughly compensates gain
                    pink6 = white * 0.115926;
                }
                break;
            case BROWN:
                while (n--) {
                    float white = ((float)((whiteSeed & 0x7fffffff) - 0x40000000)) * (float)(1.0f / 0x40000000);
                    white = (white * 0.5f) - 1.0f;
                    whiteSeed = whiteSeed * 435898247 + 382842987;
                    float brown = (lastBrown + (0.02f * white)) / 1.02f;
                    *destP++ = brown * 3.5f; // roughly compensate for gain
                    lastBrown = brown;
                }
                break;
            default:
                throw std::invalid_argument("Invalid type specified");
        }
        outputBus->clearSilentFlag();
    }

    void NoiseNode::reset(ContextRenderLock&)
    {
    }
    
    bool NoiseNode::propagatesSilence(ContextRenderLock & r) const
    {
        return !isPlayingOrScheduled() || hasFinished();
    }
    
} // namespace lab

