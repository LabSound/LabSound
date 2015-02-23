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

#include "LabSoundConfig.h"
#include "BiquadFilterNode.h"

namespace WebCore {

BiquadFilterNode::BiquadFilterNode(float sampleRate)
    : AudioBasicProcessorNode(sampleRate)
{
    // Initially setup as lowpass filter.
    m_processor = std::move(std::unique_ptr<WebCore::AudioProcessor>(new BiquadProcessor(sampleRate, 1, false)));
    setNodeType(NodeTypeBiquadFilter);
}

void BiquadFilterNode::setType(unsigned short type, ExceptionCode& ec)
{
    if (type > BiquadProcessor::Allpass) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
    
    biquadProcessor()->setType(static_cast<BiquadProcessor::FilterType>(type));
}


void BiquadFilterNode::getFrequencyResponse(ContextRenderLock& r,
                                            const std::vector<float>& frequencyHz,
                                            std::vector<float>& magResponse,
                                            std::vector<float>& phaseResponse)
{
    if (!frequencyHz.size() || !magResponse.size() || !phaseResponse.size())
        return;
    
    int n = std::min(frequencyHz.size(),
                     std::min(magResponse.size(), phaseResponse.size()));

    if (n) {
        biquadProcessor()->getFrequencyResponse(r, n, &frequencyHz[0], &magResponse[0], &phaseResponse[0]);
    }
}

} // namespace WebCore
