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

#ifndef WAVE_DECODER_H
#define WAVE_DECODER_H

#include "AudioDecoder.h"

namespace nqr
{
    
enum WaveFormatCode
{
	FORMAT_UNKNOWN = 0x0,	// Unknown Wave Format
	FORMAT_PCM = 0x1, 		// PCM Format
	FORMAT_IEEE = 0x3,		// IEEE float/double
	FORMAT_ALAW = 0x6,		// 8-bit ITU-T G.711 A-law
	FORMAT_MULAW = 0x7,		// 8-bit ITU-T G.711 µ-law
	FORMAT_EXT = 0xFFFE		// Set via subformat
};

struct RiffChunkHeader
{
	uint32_t id_riff;			// Chunk ID: 'RIFF'
	uint32_t file_size;			// Entire file in bytes
	uint32_t id_wave;			// Chunk ID: 'WAVE'
};

struct WaveChunkHeader
{
	uint32_t fmt_id;			// Chunk ID: 'fmt '
	uint32_t chunk_size;		// Size in bytes
	uint16_t format;			// Format code
	uint16_t channel_count;		// Num interleaved channels
	uint32_t sample_rate;		// SR
	uint32_t data_rate;			// Data rate
	uint16_t frame_size;		// 1 frame = channels * bits per sample (also known as block align)
	uint16_t bit_depth;			// Bits per sample	
};

struct BextChunk 
{
	uint32_t fmt_id;			// Chunk ID: 'bext'
	uint32_t chunk_size;		// Size in bytes
	uint8_t description[256];	// Description of the sound (ascii)
	uint8_t origin[32];			// Name of the originator (ascii)
	uint8_t origin_ref[32];		// Reference of the originator (ascii)
	uint8_t orgin_date[10];		// yyyy-mm-dd (ascii)
	uint8_t origin_time[8];		// hh-mm-ss (ascii)
	uint64_t time_ref;          // First sample count since midnight
	uint32_t version;			// Version of the BWF
	uint8_t uimd[64];			// Byte 0 of SMPTE UMID
	uint8_t reserved[188];		// 190 bytes, reserved for future use & set to NULL
};

struct FactChunk
{
	uint32_t fact_id;			// Chunk ID: 'fact'
	uint32_t chunk_size;		// Size in bytes 
	uint32_t sample_length;		// number of samples per channel
};

struct ExtensibleData
{
    uint16_t size;
    uint16_t valid_bits_per_sample;
    uint32_t channel_mask;
    struct GUID
    {
        uint32_t data0;
        uint16_t data1;
        uint16_t data2;
        uint16_t data3;
        uint8_t data4[6];
    };
};

template<class C, class R> 
std::basic_ostream<C,R> & operator << (std::basic_ostream<C,R> & a, const WaveChunkHeader & b) 
{ 
	return a <<
		"Format ID:\t\t"        << b.fmt_id <<
		"\nChunk Size:\t\t"     << b.chunk_size <<
		"\nFormat Code:\t\t"    << b.format <<
		"\nChannels:\t\t"       << b.channel_count <<
		"\nSample Rate:\t\t"    << b.sample_rate <<
		"\nData Rate:\t\t"      << b.data_rate <<
		"\nFrame Size:\t\t"     << b.frame_size <<
		"\nBit Depth:\t\t"      << b.bit_depth << std::endl;
}

//@todo expose speaker/channel/layout masks in the API: 
    
enum SpeakerChannelMask
{
    SPEAKER_FRONT_LEFT = 0x00000001,
    SPEAKER_FRONT_RIGHT = 0x00000002,
    SPEAKER_FRONT_CENTER = 0x00000004,
    SPEAKER_LOW_FREQUENCY = 0x00000008,
    SPEAKER_BACK_LEFT = 0x00000010,
    SPEAKER_BACK_RIGHT = 0x00000020,
    SPEAKER_FRONT_LEFT_OF_CENTER = 0x00000040,
    SPEAKER_FRONT_RIGHT_OF_CENTER = 0x00000080,
    SPEAKER_BACK_CENTER = 0x00000100,
    SPEAKER_SIDE_LEFT = 0x00000200,
    SPEAKER_SIDE_RIGHT = 0x00000400,
    SPEAKER_TOP_CENTER = 0x00000800,
    SPEAKER_TOP_FRONT_LEFT = 0x00001000,
    SPEAKER_TOP_FRONT_CENTER = 0x00002000,
    SPEAKER_TOP_FRONT_RIGHT = 0x00004000,
    SPEAKER_TOP_BACK_LEFT = 0x00008000,
    SPEAKER_TOP_BACK_CENTER = 0x00010000,
    SPEAKER_TOP_BACK_RIGHT = 0x00020000,
    SPEAKER_RESERVED = 0x7FFC0000,
    SPEAKER_ALL = 0x80000000
};

enum SpeakerLayoutMask
{
    SPEAKER_MONO = (SPEAKER_FRONT_CENTER),
    SPEAKER_STEREO = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT),
    SPEAKER_2POINT1 = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY),
    SPEAKER_SURROUND = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER),
    SPEAKER_QUAD = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT),
    SPEAKER_4POINT1 = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT),
    SPEAKER_5POINT1 = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT),
    SPEAKER_7POINT1 = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER),
    SPEAKER_5POINT1_SURROUND = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT),
    SPEAKER_7POINT1_SURROUND = (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT),
};

//@todo verify mask values
inline int ComputeChannelMask(const size_t channels)
{
	switch (channels)
	{
		case 1: 
			return SPEAKER_MONO;
		case 2: 
			return SPEAKER_STEREO;
		case 3: 
			return SPEAKER_2POINT1;
		case 4: 
			return SPEAKER_QUAD;
		case 5: 
			return SPEAKER_4POINT1;
		case 6: 
			return SPEAKER_5POINT1;
		default: 
			return -1; 
	}
}

struct WavDecoder : public nqr::BaseDecoder
{
    WavDecoder() {}
    virtual ~WavDecoder() {}
    virtual int LoadFromPath(nqr::AudioData * data, const std::string & path) override;
    virtual int LoadFromBuffer(nqr::AudioData * data, const std::vector<uint8_t> & memory) override;
    virtual std::vector<std::string> GetSupportedFileExtensions() override;
};
    
} // end namespace nqr

#endif
