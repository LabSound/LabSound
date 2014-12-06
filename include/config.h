/*
 * Copyright (C) 2004, 2005, 2006 Apple Inc.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */ 

#if defined(HAVE_CONFIG_H) && HAVE_CONFIG_H
#ifdef BUILDING_WITH_CMAKE
#include "cmakeconfig.h"
#else
#include "autotoolsconfig.h"
#endif
#endif

#include "WTF/Platform.h"

#include "WTF/ExportMacros.h"
#include "PlatformExportMacros.h"

#include "LabSoundConfig.h"

#ifdef HAVE_JS
#include <runtime/JSExportMacros.h>
#endif

#ifdef __APPLE__
#define HAVE_FUNC_USLEEP 1
#endif /* __APPLE__ */

#if OS(WINDOWS)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#ifndef WINVER
#define WINVER 0x0500
#endif

// If we don't define these, they get defined in windef.h.
// We want to use std::min and std::max.
#ifndef max
#define max max
#endif
#ifndef min
#define min min
#endif

#endif /* OS(WINDOWS) */

#ifdef __cplusplus

// These undefs match up with defines in WebCorePrefix.h for Mac OS X.
// Helps us catch if anyone uses new or delete by accident in code and doesn't include "config.h".
#undef new
#undef delete
// #include <WTF/FastMalloc.h>

#include <ciso646>

#endif

// On MSW, wx headers need to be included before windows.h is.
// The only way we can always ensure this is if we include wx here.
#if PLATFORM(WX)
#include <wx/defs.h>
#endif

//#include <wtf/DisallowCType.h>

#if COMPILER(MSVC)
#define SKIP_STATIC_CONSTRUCTORS_ON_MSVC 1
#else
#define SKIP_STATIC_CONSTRUCTORS_ON_GCC 1
#endif


// FIXME: Move this to JavaScriptCore/wtf/Platform.h, which is where we define WTF_USE_AVFOUNDATION on the Mac.
// https://bugs.webkit.org/show_bug.cgi?id=67334
#if PLATFORM(WIN) && HAVE(AVCF)
#define WTF_USE_AVFOUNDATION 1
#endif

