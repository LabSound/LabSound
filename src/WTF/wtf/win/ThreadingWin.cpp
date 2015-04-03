/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile, Inc. All rights reserved.
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

/*
 * There are numerous academic and practical works on how to implement pthread_cond_wait/pthread_cond_signal/pthread_cond_broadcast
 * functions on Win32. Here is one example: http://www.cs.wustl.edu/~schmidt/win32-cv-1.html which is widely credited as a 'starting point'
 * of modern attempts. There are several more or less proven implementations, one in Boost C++ library (http://www.boost.org) and another
 * in pthreads-win32 (http://sourceware.org/pthreads-win32/).
 *
 * The number of articles and discussions is the evidence of significant difficulties in implementing these primitives correctly.
 * The brief search of revisions, ChangeLog entries, discussions in comp.programming.threads and other places clearly documents
 * numerous pitfalls and performance problems the authors had to overcome to arrive to the suitable implementations.
 * Optimally, WebKit would use one of those supported/tested libraries directly. To roll out our own implementation is impractical,
 * if even for the lack of sufficient testing. However, a faithful reproduction of the code from one of the popular supported
 * libraries seems to be a good compromise.
 *
 * The early Boost implementation (http://www.boxbackup.org/trac/browser/box/nick/win/lib/win32/boost_1_32_0/libs/thread/src/condition.cpp?rev=30)
 * is identical to pthreads-win32 (http://sourceware.org/cgi-bin/cvsweb.cgi/pthreads/pthread_cond_wait.c?rev=1.10&content-type=text/x-cvsweb-markup&cvsroot=pthreads-win32).
 * Current Boost uses yet another (although seemingly equivalent) algorithm which came from their 'thread rewrite' effort.
 *
 * This file includes timedWait/signal/broadcast implementations translated to WebKit coding style from the latest algorithm by
 * Alexander Terekhov and Louis Thomas, as captured here: http://sourceware.org/cgi-bin/cvsweb.cgi/pthreads/pthread_cond_wait.c?rev=1.10&content-type=text/x-cvsweb-markup&cvsroot=pthreads-win32
 * It replaces the implementation of their previous algorithm, also documented in the same source above.
 * The naming and comments are left very close to original to enable easy cross-check.
 *
 * The corresponding Pthreads-win32 License is included below, and CONTRIBUTORS file which it refers to is added to
 * source directory (as CONTRIBUTORS.pthreads-win32).
 */

/*
 *      Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2005 Pthreads-win32 contributors
 *
 *      Contact Email: rpj@callisto.canberra.edu.au
 *
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "LabSoundConfig.h"

#include "Threading.h"
#include "MainThread.h"
#include "ThreadFunctionInvocation.h"

#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>

#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

#if !USE(PTHREADS) && OS(WINDOWS)
#include "ThreadSpecific.h"
#include <windows.h>
#endif

#if !OS(WINCE)
#include <process.h>
#endif

#if HAVE(ERRNO_H)
#include <errno.h>
#endif

namespace WTF {

// MS_VC_EXCEPTION, THREADNAME_INFO, and setThreadNameInternal all come from <http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx>.
static const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
    DWORD dwType; // must be 0x1000
    LPCSTR szName; // pointer to name (in user addr space)
    DWORD dwThreadID; // thread ID (-1=caller thread)
    DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;
#pragma pack(pop)

void initializeCurrentThreadInternal(const char* szThreadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = szThreadName;
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;

    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info));
    } 
    __except (EXCEPTION_CONTINUE_EXECUTION) {
    }
}

static std::mutex* atomicallyInitializedStaticMutex;

void lockAtomicallyInitializedStaticMutex()
{
    ASSERT(atomicallyInitializedStaticMutex);
    atomicallyInitializedStaticMutex->lock();
}

void unlockAtomicallyInitializedStaticMutex()
{
    atomicallyInitializedStaticMutex->unlock();
}

static std::mutex& threadMapMutex()
{
    static std::mutex mutex;
    return mutex;
}

void initializeThreading()
{
    if (atomicallyInitializedStaticMutex)
        return;

    atomicallyInitializedStaticMutex = new std::mutex;
    threadMapMutex();

    // wtfThreadData();

}

static std::map<DWORD, HANDLE>& threadMap()
{
    static std::map<DWORD, HANDLE> map;
    return map;
}

static void storeThreadHandleByIdentifier(DWORD threadID, HANDLE threadHandle)
{
    std::lock_guard<std::mutex> locker(threadMapMutex());
    ASSERT(!threadMap().count(threadID));
    threadMap().insert(std::make_pair(threadID, threadHandle));
}

static HANDLE threadHandleForIdentifier(ThreadIdentifier id)
{
    std::lock_guard<std::mutex> locker(threadMapMutex());
    return threadMap().find(id)->second;
}

static void clearThreadHandleForIdentifier(ThreadIdentifier id)
{
    std::lock_guard<std::mutex> locker(threadMapMutex());
    ASSERT(threadMap().count(id));
    threadMap().erase(id);
}

static unsigned __stdcall wtfThreadEntryPoint(void* param)
{
    std::unique_ptr<ThreadFunctionInvocation> invocation(static_cast<ThreadFunctionInvocation*>(param));
    invocation->function(invocation->data);

#if !USE(PTHREADS) && OS(WINDOWS)
    // Do the TLS cleanup.
    ThreadSpecificThreadExit();
#endif

    return 0;
}

ThreadIdentifier createThreadInternal(ThreadFunction entryPoint, void* data, const char* threadName)
{
    unsigned threadIdentifier = 0;
    ThreadIdentifier threadID = 0;
    std::unique_ptr<ThreadFunctionInvocation> invocation(new ThreadFunctionInvocation(entryPoint, data));

    HANDLE threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, wtfThreadEntryPoint, invocation.get(), 0, &threadIdentifier));

    if (!threadHandle) 
	{
		LOG_ERROR("Failed to create thread at entry point %p with data %p.", entryPoint, data);
        return 0;
    }

    // The thread will take ownership of invocation.
    invocation.release();

    threadID = static_cast<ThreadIdentifier>(threadIdentifier);
    storeThreadHandleByIdentifier(threadIdentifier, threadHandle);

    return threadID;
}

int waitForThreadCompletion(ThreadIdentifier threadID)
{
    ASSERT(threadID);
    
    HANDLE threadHandle = threadHandleForIdentifier(threadID);
    if (!threadHandle)
        LOG_ERROR("ThreadIdentifier %u did not correspond to an active thread when trying to quit", threadID);
 
    DWORD joinResult = WaitForSingleObject(threadHandle, INFINITE);
    if (joinResult == WAIT_FAILED)
        LOG_ERROR("ThreadIdentifier %u was found to be deadlocked trying to quit", threadID);

    CloseHandle(threadHandle);
    clearThreadHandleForIdentifier(threadID);

    return joinResult;
}

void detachThread(ThreadIdentifier threadID)
{
    ASSERT(threadID);

    HANDLE threadHandle = threadHandleForIdentifier(threadID);
    if (threadHandle)
        CloseHandle(threadHandle);
    clearThreadHandleForIdentifier(threadID);
}

void yield()
{
    ::Sleep(1);
}

ThreadIdentifier currentThread()
{
    return static_cast<ThreadIdentifier>(GetCurrentThreadId());
}



static const long MaxSemaphoreCount = static_cast<long>(~0UL >> 1);


DWORD absoluteTimeToWaitTimeoutInterval(double absoluteTime)
{
    double currentTime = WTF::currentTime();

    // Time is in the past - return immediately.
    if (absoluteTime < currentTime)
        return 0;

    // Time is too far in the future (and would overflow unsigned long) - wait forever.
    if (absoluteTime - currentTime > static_cast<double>(INT_MAX) / 1000.0)
        return INFINITE;

    return static_cast<DWORD>((absoluteTime - currentTime) * 1000.0);
}

} // namespace WTF