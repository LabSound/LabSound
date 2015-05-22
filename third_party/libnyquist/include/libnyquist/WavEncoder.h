/*
Copyright (c) 2015, Dimitri Diakopoulos All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef WAVE_ENCODER_H
#define WAVE_ENCODER_H

#include "Common.h"
#include "WavDecoder.h"
#include "RiffUtils.h"

namespace nqr
{

    
// A simplistic encoder that takes a blob of data, conforms it to the user's
// EncoderParams preference, and writes to disk. Be warned, does not support resampling!
// @todo support dithering, samplerate conversion, etc.
class WavEncoder
{
    enum EncoderError
    {
        NoError,
        InsufficientSampleData,
        FileIOError,
        UnsupportedSamplerate,
        UnsupportedChannelConfiguration,
        UnsupportedBitdepth,
        UnsupportedChannelMix,
        BufferTooBig,
    };
    
public:
    
    WavEncoder();
    ~WavEncoder();
    
    // Assume data adheres to EncoderParams, except for bit depth and fmt
    int WriteFile(const EncoderParams p, const AudioData * d, const std::string & path);
    
};
    
} // end namespace nqr

#endif
