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

#ifndef RIFF_UTILS_H
#define RIFF_UTILS_H

#include "Common.h"
#include "WavDecoder.h"
#include "Dither.h"

namespace nqr
{
    
/////////////////////
// Chunk utilities //
/////////////////////

struct EncoderParams
{
    int channelCount;
    PCMFormat targetFormat;
    DitherType dither;
};

struct ChunkHeaderInfo
{
    uint32_t offset;			// Byte offset into chunk
    uint32_t size;				// Size of the chunk in bytes
};

inline uint32_t GenerateChunkCode(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    #ifdef ARCH_CPU_LITTLE_ENDIAN
        return ((uint32_t) ((a) | ((b) << 8) | ((c) << 16) | (((uint32_t) (d)) << 24)));
    #else
        return ((uint32_t) ((((uint32_t) (a)) << 24) | ((b) << 16) | ((c) << 8) | (d)));
    #endif
}
    
inline char * GenerateChunkCodeChar(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
auto chunk = GenerateChunkCode(a, b, c, d);
    
   char * outArr = new char[4];
    
    uint32_t t = 0x000000FF;
    
    for(size_t i = 0; i < 4; i++)
    {
        outArr[i] = chunk & t;
        chunk >>= 8;
    }
    return outArr;
}

ChunkHeaderInfo ScanForChunk(const std::vector<uint8_t> & fileData, uint32_t chunkMarker);
    
WaveChunkHeader MakeWaveHeader(const EncoderParams param, const int sampleRate);

} // end namespace nqr

#endif
