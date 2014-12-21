
// Copyright (c) 2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT
#pragma once

#include "AudioBasicInspectorNode.h"
#include "AudioContext.h"
#include <vector>
#include <mutex>

namespace LabSound {

    class SpectralMonitorNode : public WebCore::AudioBasicInspectorNode {
    public:
        static WTF::PassRefPtr<SpectralMonitorNode> create(std::shared_ptr<WebCore::AudioContext> context, float sampleRate)
        {
            return adoptRef(new SpectralMonitorNode(context, sampleRate));
        }

        virtual ~SpectralMonitorNode();

        // AudioNode
        virtual void process(size_t framesToProcess);
        virtual void reset();
        // ..AudioNode

        void spectralMag(std::vector<float>& result);
        void windowSize(size_t ws);
        size_t windowSize() const;

    private:
        SpectralMonitorNode(std::shared_ptr<WebCore::AudioContext>, float sampleRate);
        class Detail;
        Detail* detail;

        // required for BasicInspector
        virtual double tailTime() const OVERRIDE { return 0; }
        virtual double latencyTime() const OVERRIDE { return 0; }
    };

} // LabSound
