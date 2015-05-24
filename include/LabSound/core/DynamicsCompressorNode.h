/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DynamicsCompressorNode_h
#define DynamicsCompressorNode_h

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"

namespace WebCore {

class DynamicsCompressor;

class DynamicsCompressorNode : public AudioNode
{
    
public:
    
    DynamicsCompressorNode(float sampleRate);
    virtual ~DynamicsCompressorNode();

    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;
    virtual void initialize();
    virtual void uninitialize();

    // Static compression curve parameters.
    std::shared_ptr<AudioParam> threshold() { return m_threshold; }
    std::shared_ptr<AudioParam> knee() { return m_knee; }
    std::shared_ptr<AudioParam> ratio() { return m_ratio; }
    std::shared_ptr<AudioParam> attack() { return m_attack; }
    std::shared_ptr<AudioParam> release() { return m_release; }

    // Amount by which the compressor is currently compressing the signal in decibels.
    std::shared_ptr<AudioParam> reduction() { return m_reduction; }

private:
    virtual double tailTime() const override;
    virtual double latencyTime() const override;

    std::unique_ptr<DynamicsCompressor> m_dynamicsCompressor;
    std::shared_ptr<AudioParam> m_threshold;
    std::shared_ptr<AudioParam> m_knee;
    std::shared_ptr<AudioParam> m_ratio;
    std::shared_ptr<AudioParam> m_reduction;
    std::shared_ptr<AudioParam> m_attack;
    std::shared_ptr<AudioParam> m_release;
};

} // namespace WebCore

#endif // DynamicsCompressorNode_h
