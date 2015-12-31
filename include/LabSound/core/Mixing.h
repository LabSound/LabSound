// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef Mixing_h
#define Mixing_h

#include <stdint.h>

namespace lab
{
    
    enum class ChannelInterpretation
    {
        Speakers,
        Discrete,
    };
    
    enum class Channel : int
    {
        First = 0,
        Left = 0,
        Right = 1,
        Center = 2, // center and mono are the same
        Mono = 2,
        LFE = 3,
        SurroundLeft = 4,
        SurroundRight = 5,
    };
    
    enum class ChannelCountMode
    {
        Max,
        ClampedMax,
        Explicit,
        End
    };

}

#endif
