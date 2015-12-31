// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef _Internal_Config_Macros_
#define _Internal_Config_Macros_

#include <WTF/Platform.h>

#include "LabSound/extended/Logging.h"
#include "internal/Assertions.h"

#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <memory>
#include <math.h>

// All 64 bit processors support SSE2
#if ((_M_IX86_FP) && (_M_IX86_FP >= 2)) || (_M_AMD64) || defined(_M_X64)
#define __SSE2__
#endif

#if OS(MAC_OS_X) || OS(IOS)
#ifndef HAVE_LIBDL
#define HAVE_LIBDL 1
#endif
#ifndef HAVE_ALLOCA
#define HAVE_ALLOCA 1
#endif
#ifndef HAVE_UNISTED_H
#define HAVE_UNISTD_H 1
#endif
#ifndef USEAPI_DUMMY
#define USEAPI_DUMMY 1
#endif
#ifndef ENABLE_MEDIA_STREAM
#define ENABLE_MEDIA_STREAM 1
#endif
#endif

#if OS(WINDOWS)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#ifndef WINVER
#define WINVER 0x0500
#endif
#endif

#endif
