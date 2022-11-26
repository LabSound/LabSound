// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef LABSOUND_UTIL_H
#define LABSOUND_UTIL_H

#include <algorithm>
#include <random>
#include <string>

// clang-format off
#define NO_COPY(C) C(const C &) = delete; C & operator = (const C &) = delete
#define NO_MOVE(C) NO_COPY(C); C(C &&) = delete; C & operator = (const C &&) = delete

#if defined(_MSC_VER)
#   if defined(min)
#       undef min
#   endif
#   if defined(max)
#       undef max
#   endif
#endif

namespace lab
{

class UniformRandomGenerator
{
    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_real_distribution<float> dist_full {0.f, 1.f};
    
public:
    UniformRandomGenerator() : rd(), gen(rd()) {}
    float random_float() { return dist_full(gen); }  // [0.f, 1.f]
    float random_float(float b) { std::uniform_real_distribution<float> dist_user(0.f, b); return dist_user(gen); }
    float random_float(float a, float b) { std::uniform_real_distribution<float> dist_user(a, b);  return dist_user(gen); }
    uint32_t random_uint(uint32_t b) { std::uniform_int_distribution<uint32_t> dist_int(0, b); return dist_int(gen); }
    int32_t random_int(int32_t a, int32_t b) { std::uniform_int_distribution<int32_t> dist_int(a, b); return dist_int(gen); }
};
// clang-format on

template <typename T>
inline T RoundNextPow2(T v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

template <typename S, typename T>
inline T clampTo(S value, T a, T b)
{
    if (value >= static_cast<S>(b)) return b;
    if (value <= static_cast<S>(a)) return a;
    return static_cast<T>(value);
}

// easier to redeclare than to force consuming projects to deal with NOMINMAX on Windows
template <typename T>
inline T Min(T a, T b)
{
    return (a < b ? a : b);
}

template <typename T>
inline T Max(T a, T b)
{
    return (a > b ? a : b);
}

} //lab

#endif
