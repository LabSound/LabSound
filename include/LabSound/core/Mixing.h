// LabSound Mixing.h
// Copyright (c) 2003-2015 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef Mixing_h
#define Mixing_h

namespace LabSound
{
    
    enum class ChannelInterpretation
    {
        Speakers,
        Discrete,
    };
    
    enum class Channel
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
