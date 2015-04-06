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

#ifndef HRTFDatabaseLoader_h
#define HRTFDatabaseLoader_h

#include "HRTFDatabase.h"
#include <thread>
#include <mutex>
#include <condition_variable>

namespace WebCore {

// HRTFDatabaseLoader will asynchronously load the default HRTFDatabase in a new thread.

class HRTFDatabaseLoader {
public:
    // Both constructor and destructor must be called from the main thread.
    // It's expected that the singletons will be accessed instead.
    // @CBB the guts of the loader should be a private singleton, so that the loader can be constructed without a factory
    explicit HRTFDatabaseLoader(float sampleRate);
    
    // Lazily creates the singleton HRTFDatabaseLoader (if not already created) and starts loading asynchronously (when created the first time).
    // Returns the singleton HRTFDatabaseLoader.
    // Must be called from the main thread.
    static std::shared_ptr<HRTFDatabaseLoader> createAndLoadAsynchronouslyIfNecessary(float sampleRate);

    // Returns the singleton HRTFDatabaseLoader.
    static std::shared_ptr<HRTFDatabaseLoader> loader() { return s_loader; }
    
    // Both constructor and destructor must be called from the main thread.
    ~HRTFDatabaseLoader();
    
    // Returns true once the default database has been completely loaded.
    bool isLoaded() const;

    // waitForLoaderThreadCompletion() may be called more than once and is thread-safe.
    void waitForLoaderThreadCompletion();
    
    HRTFDatabase* database() { return m_hrtfDatabase.get(); }

    float databaseSampleRate() const { return m_databaseSampleRate; }
    
    // Called in asynchronous loading thread.
    void load();

    // defaultHRTFDatabase() gives access to the loaded database.
    // This can be called from any thread, but it is the callers responsibilty to call this while the context (and thus HRTFDatabaseLoader)
    // is still alive.  Otherwise this will return 0.
    static HRTFDatabase* defaultHRTFDatabase();

private:
    static void databaseLoaderEntry(HRTFDatabaseLoader* threadData);

    // If it hasn't already been loaded, creates a new thread and initiates asynchronous loading of the default database.
    // This must be called from the main thread.
    void loadAsynchronously();

    static std::shared_ptr<HRTFDatabaseLoader> s_loader; // singleton
    std::unique_ptr<HRTFDatabase> m_hrtfDatabase;

    // Holding a m_threadLock is required when accessing m_databaseLoaderThread.
    std::mutex m_threadLock;
    std::thread m_databaseLoaderThread;
    std::condition_variable m_loadingCondition;
    bool m_loading;

    float m_databaseSampleRate;
};

} // namespace WebCore

#endif // HRTFDatabaseLoader_h
