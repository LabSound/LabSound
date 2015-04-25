/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
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

#ifndef AudioSummingJunction_h
#define AudioSummingJunction_h

#include <vector>
#include <memory>

namespace LabSound 
{
    class ContextGraphLock;
    class ContextRenderLock;
}

namespace WebCore {

    class AudioContext;
    class AudioNodeOutput;
	class AudioBus;
    
    using namespace LabSound;

// An AudioSummingJunction represents a point where zero, one, or more AudioNodeOutputs connect.

class AudioSummingJunction {
public:
    explicit AudioSummingJunction();
    virtual ~AudioSummingJunction();

    // This must be called whenever we modify m_outputs.
    void changedOutputs(ContextGraphLock&);
    
    // This copies m_outputs to m_renderingOutputs. See comments for these lists below.
    void updateRenderingState(ContextRenderLock& r);

    size_t numberOfConnections() const {
        return m_connectedOutputs.size();   // will count expired pointers
    }
    
    // Rendering code accesses its version of the current connections here.
    size_t numberOfRenderingConnections(ContextRenderLock&) const;
    std::shared_ptr<AudioNodeOutput> renderingOutput(ContextRenderLock&, unsigned i) {
        return i < m_renderingOutputs.size() ? m_renderingOutputs[i].lock() : nullptr; }
    
    const std::shared_ptr<AudioNodeOutput> renderingOutput(ContextRenderLock&, unsigned i) const {
        return i < m_renderingOutputs.size() ? m_renderingOutputs[i].lock() : nullptr; }
    
    bool isConnected() const { return numberOfConnections() > 0; }

    virtual bool canUpdateState() = 0;
    virtual void didUpdate(ContextRenderLock&) = 0;

    void junctionConnectOutput(std::shared_ptr<AudioNodeOutput>);
    void junctionDisconnectOutput(std::shared_ptr<AudioNodeOutput>);
    void setDirty() { m_renderingStateNeedUpdating = true; }
    
    static void handleDirtyAudioSummingJunctions(ContextRenderLock& r);

    bool isConnected(std::shared_ptr<AudioNodeOutput> o) const;

    
private:
    // m_outputs contains the AudioNodeOutputs representing current connections.
    // The rendering code should never use this directly, but instead uses m_renderingOutputs.
    std::vector<std::weak_ptr<AudioNodeOutput>> m_connectedOutputs;

    // m_renderingOutputs is a copy of m_connectedOutputs which will never be modified during the graph rendering on the audio thread.
    // This is the list which is used by the rendering code.
    // Whenever m_outputs is modified, the context is told so it can later update m_renderingOutputs from m_outputs at a safe time.
    // Most of the time, m_renderingOutputs is identical to m_outputs.
    std::vector<std::weak_ptr<AudioNodeOutput>> m_renderingOutputs;

    // m_renderingStateNeedUpdating indicates outputs were changed
    bool m_renderingStateNeedUpdating;
};

} // namespace WebCore

#endif // AudioSummingJunction_h
