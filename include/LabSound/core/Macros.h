// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef labsound_macros_h
#define labsound_macros_h

#if (defined(__linux) || defined(__unix) || defined(__posix) || defined(__LINUX__) || defined(__linux__))
#define LABSOUND_PLATFORM_LINUX 1
#elif (defined(_WIN64) || defined(_WIN32) || defined(__CYGWIN32__) || defined(__MINGW32__))
#define LABSOUND_PLATFORM_WINDOWS 1
#elif (defined(MACOSX) || defined(__DARWIN__) || defined(__APPLE__))
#define LABSOUND_PLATFORM_OSX 1
#endif

#if (defined(WIN_32) || defined(__i386__) || defined(i386) || defined(__x86__))
#define LABSOUND_ARCH_32 1
#elif (defined(__amd64) || defined(__amd64__) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64__) || defined(_M_IA64))
#define LABSOUND_ARCH_64 1
#endif

#if (defined(__clang__))
#define LABSOUND_COMPILER_CLANG 1
#elif (defined(__GNUC__))
#define LABSOUND_COMPILER_GCC 1
#elif (defined _MSC_VER)
#define LABSOUND_COMPILER_VISUAL_STUDIO 1
#endif

#if ((_M_IX86_FP) && (_M_IX86_FP >= 2)) || (_M_AMD64) || defined(_M_X64)
#define __SSE2__
#endif

#if defined(LABSOUND_COMPILER_VISUAL_STUDIO)
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#endif

#if defined(__ARM_NEON__)
#define ARM_NEON_INTRINSICS 1
#endif

#if defined(LABSOUND_PLATFORM_OSX) || defined(LABSOUND_PLATFORM_LINUX)
#include <math.h>
#include <cmath>
#endif

#if defined(LABSOUND_PLATFORM_WINDOWS)
    #define WEBAUDIO_KISSFFT
#endif

const double piDouble = M_PI;
const float piFloat = static_cast<float>(M_PI);

const double piOverTwoDouble = M_PI_2;
const float piOverTwoFloat = static_cast<float>(M_PI_2);

const double piOverFourDouble = M_PI_4;
const float piOverFourFloat = static_cast<float>(M_PI_4);

const double twoPiDouble = piDouble * 2.0;
const float twoPiFloat = piFloat * 2.0f;

template<typename T> inline T clampTo(double value, T min, T max)
{
    if (value >= static_cast<double>(max)) return max;
    if (value <= static_cast<double>(min)) return min;
    return static_cast<T>(value);
}

#endif // end labsound_macros_h
