// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioFileReader_H
#define AudioFileReader_H

#include "LabSound/core/AudioBus.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <memory>
#include <vector>
#include <string>
#include <stdint.h>
#include <fstream>
#include <iterator>

#define SWR_CH_MAX 64

// Allocator adaptor that interposes construct() calls to
// convert value initialization into default initialization.
template <typename T, typename A=std::allocator<T>>
class default_init_allocator : public A {
  typedef std::allocator_traits<A> a_t;
public:
  template <typename U> struct rebind {
    using other =
      default_init_allocator<
        U, typename a_t::template rebind_alloc<U>
      >;
  };

  using A::A;

  template <typename U>
  void construct(U* ptr)
    noexcept(std::is_nothrow_default_constructible<U>::value) {
    ::new(static_cast<void*>(ptr)) U;
  }
  template <typename U, typename...Args>
  void construct(U* ptr, Args&&... args) {
    a_t::construct(static_cast<A&>(*this),
                   ptr, std::forward<Args>(args)...);
  }
};

typedef std::vector<float, default_init_allocator<float>> Plane;
typedef std::vector<Plane> PlanesVector;

namespace detail {
  class AppData {
  public:
    AppData();
    ~AppData();

    void resetState();
    bool set(std::vector<uint8_t> &memory, std::string *error = nullptr);
    static int bufferRead(void *opaque, unsigned char *buf, int buf_size);
    static int64_t bufferSeek(void *opaque, int64_t offset, int whence);
    PlanesVector read(std::string *error = nullptr);
    float getSampleRate();

  public:
    std::vector<uint8_t> data;
    int64_t dataPos;

    AVFormatContext *fmt_ctx;
    AVIOContext *io_ctx;
    int stream_idx;
    AVStream *audio_stream;
    AVCodecContext *codec_ctx;
    AVCodec *decoder;
    AVPacket packet;
    AVFrame *av_frame;
    SwrContext *swr_ctx;
    uint8_t *sr_planes[SWR_CH_MAX];
  };
}
namespace lab {
  std::unique_ptr<AudioBus> MakeBusFromFile(const char *filePath, bool mixToMono, std::string *error = nullptr);
  std::unique_ptr<AudioBus> MakeBusFromMemory(std::vector<uint8_t> &buffer, bool mixToMono, std::string *error = nullptr);
  std::unique_ptr<AudioBus> MakeBusFromRawBuffer(size_t sampleRate, size_t numChannels, size_t numSamples, float *frames[], bool mixToMono, std::string *error = nullptr);
}

#endif
