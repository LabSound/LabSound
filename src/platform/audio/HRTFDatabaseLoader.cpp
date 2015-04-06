/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSoundConfig.h"
#include "HRTFDatabaseLoader.h"

#include "HRTFDatabase.h"
#include <iostream>

namespace WebCore {

// Singleton
std::shared_ptr<HRTFDatabaseLoader> HRTFDatabaseLoader::s_loader;

std::shared_ptr<HRTFDatabaseLoader> HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(float sampleRate)
{
    if (!s_loader)
    {
        s_loader = std::make_shared<HRTFDatabaseLoader>(sampleRate);
        s_loader->loadAsynchronously();
    }
    return s_loader;
}

HRTFDatabaseLoader::HRTFDatabaseLoader(float sampleRate) : m_databaseSampleRate(sampleRate), m_loading(false)
{
    ASSERT(!s_loader.get());
}

HRTFDatabaseLoader::~HRTFDatabaseLoader()
{
    waitForLoaderThreadCompletion();
    
    if (m_databaseLoaderThread.joinable()) m_databaseLoaderThread.join();

    m_hrtfDatabase.reset();
    
    ASSERT(this == s_loader.get());
    
    s_loader.reset();
}


// Asynchronously load the database in this thread.
void HRTFDatabaseLoader::databaseLoaderEntry(HRTFDatabaseLoader* threadData)
{
    std::lock_guard<std::mutex> locker(threadData->m_threadLock);
    HRTFDatabaseLoader* loader = reinterpret_cast<HRTFDatabaseLoader*>(threadData);
    ASSERT(loader);
    threadData->m_loading = true;
    loader->load();
    threadData->m_loadingCondition.notify_one();
}

void HRTFDatabaseLoader::load()
{
    m_hrtfDatabase = HRTFDatabase::create(m_databaseSampleRate);
    
    if (!m_hrtfDatabase.get())
    {
        std::cerr << "HRTF database not loaded..." << std::endl;
    }
}

void HRTFDatabaseLoader::loadAsynchronously()
{
    std::unique_lock<std::mutex> lock(m_threadLock);
    
    if (!m_hrtfDatabase.get() && !m_loading)
    {
        m_databaseLoaderThread = std::thread(databaseLoaderEntry, this);
    }
}

bool HRTFDatabaseLoader::isLoaded() const
{
    return !!m_hrtfDatabase.get();
}

void HRTFDatabaseLoader::waitForLoaderThreadCompletion()
{
    std::unique_lock<std::mutex> locker(m_threadLock);
    while (!m_hrtfDatabase.get())
        m_loadingCondition.wait(locker);
}

HRTFDatabase* HRTFDatabaseLoader::defaultHRTFDatabase()
{
    if (!s_loader)
        return 0;
    
    return s_loader->database();
}

} // namespace WebCore
