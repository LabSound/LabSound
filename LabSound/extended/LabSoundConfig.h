#ifndef Labsound_Config_h
#define Labsound_Config_h

#include "WTF/Platform.h"
#include "WTF/ExportMacros.h"

#include "PlatformExportMacros.h"
#include "Logging.h"
#include "Assertions.h"

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
