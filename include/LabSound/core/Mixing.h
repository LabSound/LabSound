
#pragma once

namespace LabSound {
    
    enum class ChannelInterpretation {
        Speakers,
        Discrete,
    };
    
    enum class Channel {
        First = 0,
        Left = 0,
        Right = 1,
        Center = 2, // center and mono are the same
        Mono = 2,
        LFE = 3,
        SurroundLeft = 4,
        SurroundRight = 5,
    };
    
    enum class ChannelCountMode {
        Max,
        ClampedMax,
        Explicit,
        End
    };

}

