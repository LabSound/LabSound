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

#ifndef AsyncAudioDecoder_h
#define AsyncAudioDecoder_h

#include "WTF/MessageQueue.h"
#include "WTF/RefPtr.h"
#include "WTF/Threading.h"
#include <mutex>
#include <vector>

namespace WebCore {

class AudioBuffer;
class AudioBufferCallback;

using namespace WTF;

// AsyncAudioDecoder asynchronously decodes audio file data from an vector<uint_8> in a worker thread.
// Upon successful decoding, a completion callback will be invoked with the decoded PCM data in an AudioBuffer.

class AsyncAudioDecoder {
    AsyncAudioDecoder(const AsyncAudioDecoder&); // noncopyable
public:
    AsyncAudioDecoder();
    ~AsyncAudioDecoder();

    // Must be called on the main thread.
    void decodeAsync(std::shared_ptr<std::vector<uint8_t>> audioData, float sampleRate, PassRefPtr<AudioBufferCallback> successCallback, PassRefPtr<AudioBufferCallback> errorCallback);

private:
    class DecodingTask {
        DecodingTask(const DecodingTask&); // noncopyable
    public:
        static std::unique_ptr<AsyncAudioDecoder::DecodingTask> create(std::shared_ptr<std::vector<uint8_t>> audioData,
                                                                       float sampleRate,
                                                                       PassRefPtr<AudioBufferCallback> successCallback,
                                                                       PassRefPtr<AudioBufferCallback> errorCallback);
        void decode();
        
    private:
        DecodingTask(std::shared_ptr<std::vector<uint8_t>> audioData, float sampleRate, PassRefPtr<AudioBufferCallback> successCallback, PassRefPtr<AudioBufferCallback> errorCallback);

        std::shared_ptr<std::vector<uint8_t>> audioData() { return m_audioData; }
        float sampleRate() const { return m_sampleRate; }
        AudioBufferCallback* successCallback() { return m_successCallback.get(); }
        AudioBufferCallback* errorCallback() { return m_errorCallback.get(); }
        std::shared_ptr<AudioBuffer> audioBuffer() { return m_audioBuffer; }

        static void notifyCompleteDispatch(void* userData);
        void notifyComplete();

        std::shared_ptr<std::vector<uint8_t>> m_audioData;
        float m_sampleRate;
        RefPtr<AudioBufferCallback> m_successCallback;
        RefPtr<AudioBufferCallback> m_errorCallback;
        std::shared_ptr<AudioBuffer> m_audioBuffer;
    };
    
    static void threadEntry(void* threadData);
    void runLoop();

    WTF::ThreadIdentifier m_threadID;
    std::mutex m_threadCreationMutex;
    MessageQueue<DecodingTask> m_queue;
};

} // namespace WebCore

#endif // AsyncAudioDecoder_h
