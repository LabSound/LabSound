//
//  AudioContextLock.h
//  LabSound
//
// Copyright (c) 2015 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT
//

#pragma once

#include "LabSound/core/AudioContext.h"

#include <iostream>
#include <mutex>

namespace LabSound {

    class ContextGraphLock {
    public:
        ContextGraphLock(std::shared_ptr<WebCore::AudioContext> context, const char* locker) {
            if (context && context->m_graphLock.try_lock()) {
                m_context = context;
                m_context->m_graphLocker = locker;
            }
            else if (context->m_graphLocker) {
               std::cerr << locker << " failed to acquire graph lock, it's already held by " << context->m_graphLocker << std::endl;
            }
            else {
               //std::cerr << locker << " failed to acquire graph lock" << std::endl;
            }
        }
        
        ~ContextGraphLock()
        {
            if (m_context)
                m_context->m_graphLock.unlock();
        }
        
        WebCore::AudioContext* context() { return m_context.get(); }
        std::shared_ptr<WebCore::AudioContext> contextPtr() { return m_context; }
        
    private:
        std::shared_ptr<WebCore::AudioContext> m_context;
    };
    
    class ContextRenderLock {
    public:
        ContextRenderLock(std::shared_ptr<WebCore::AudioContext> context, const char* locker) {
            if (context && context->m_renderLock.try_lock()) {
                m_context = context;
                m_context->m_renderLocker = locker;
            }
            else if (context->m_graphLocker) {
               std::cerr << locker << " failed to acquire graph lock, it's already held by " << m_context->m_graphLocker << std::endl;
            }
            else {
                //std::cerr << locker << " failed to acquire graph lock" << std::endl;
            }
        }
        
        ~ContextRenderLock()
        {
            if (m_context)
                m_context->m_renderLock.unlock();
        }
        
        WebCore::AudioContext* context() { return m_context.get(); }
        std::shared_ptr<WebCore::AudioContext> contextPtr() { return m_context; }
        
    private:
        std::shared_ptr<WebCore::AudioContext> m_context;
    };

}
