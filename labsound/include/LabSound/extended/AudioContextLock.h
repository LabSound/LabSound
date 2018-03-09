// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

//#define DEBUG_LOCKS

#ifndef AUDIO_CONTEXT_LOCK_H
#define AUDIO_CONTEXT_LOCK_H

#include "LabSound/core/AudioContext.h"
#include "LabSound/extended/Logging.h"

#include <iostream>
#include <mutex>

//#define DEBUG_LOCKS

namespace lab
{

    class ContextGraphLock
    {
        AudioContext * m_context;

    public:
        
        ContextGraphLock(AudioContext * context, const std::string & lockSuitor)
        {
            if (context)
            {
                context->m_graphLock.lock();
                m_context = context;
                m_context->m_graphLocker = lockSuitor;
            }
#if defined(DEBUG_LOCKS)
            if (!m_context && context->m_graphLocker.size())
            {
                LOG("%s failed to acquire [GRAPH] lock. Currently held by: %s.", lockSuitor.c_str(), context->m_graphLocker.c_str());
            }
#endif
        }
        
        ~ContextGraphLock()
        {
            if (m_context)
            {
                //m_context->m_graphLocker.clear();
                m_context->m_graphLock.unlock();
            }
            
        }
        
        AudioContext * context() { return m_context; }
    };
    
    class ContextRenderLock
    {
        AudioContext * m_context;

    public:
        
        ContextRenderLock(AudioContext * context, const std::string & lockSuitor)
        {
            if (context)
            {
                context->m_renderLock.lock();
                m_context = context;
                m_context->m_renderLocker = lockSuitor;
            }
#if defined(DEBUG_LOCKS)
            else if (context && context->m_renderLocker.size())
            {
                LOG("%s failed to acquire [RENDER] lock. Currently held by: %s.", lockSuitor.c_str(), context->m_renderLocker.c_str());
            }
            else
            {
                LOG("%s failed to acquire [RENDER] lock.", lockSuitor.c_str());
            }
#endif
        }
        
        ~ContextRenderLock()
        {
            if (m_context)
            {
                m_context->m_renderLock.unlock();
            }
        }
        
        AudioContext * context() { return m_context; }
    };

    class ContextRenderUnlock
    {
        AudioContext * m_context;

    public:
        
        ContextRenderUnlock(AudioContext * context)
        {
            if (context)
            {
                context->m_renderLock.unlock();
                m_context = context;
            }
        }
        
        ~ContextRenderUnlock()
        {
            if (m_context)
            {
                m_context->m_renderLock.lock();
            }
        }
        
        AudioContext * context() { return m_context; }
    };

} // end namespace lab

#endif
