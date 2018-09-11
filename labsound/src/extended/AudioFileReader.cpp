// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/Assertions.h"

#include "LabSound/core/Macros.h"
#include "LabSound/core/AudioBus.h"

#include "LabSound/extended/AudioFileReader.h"

#include <mutex>
#include <cstring>
#include <algorithm>

/**
 * Original LabSound decoder used libnyquist which did not have as wide codec support as ffmpeg.
 * This fork uses ffmpeg.
 */
namespace detail
{
    const size_t kBufferSize = 4 * 1024;
    const size_t kPlaneBufferSize = 512 * 1024;
    const AVPixelFormat kPixelFormat = AV_PIX_FMT_RGBA;

    AppData::AppData() :
      dataPos(0),
      fmt_ctx(nullptr), io_ctx(nullptr), stream_idx(-1), audio_stream(nullptr), codec_ctx(nullptr), decoder(nullptr), av_frame(nullptr), swr_ctx(nullptr)
    {
      av_init_packet(&packet);
      packet.data = nullptr;
      packet.size = 0;
      for (size_t i = 0; i < SWR_CH_MAX; i++) {
        sr_planes[i] = nullptr;
      }
      sr_planes[0] = (uint8_t *)malloc(kPlaneBufferSize);
      sr_planes[1] = (uint8_t *)malloc(kPlaneBufferSize);
    }
    AppData::~AppData() {
      resetState();
      free(sr_planes[0]);
      free(sr_planes[1]);
    }

    void AppData::resetState() {
      if (av_frame) {
        av_free(av_frame);
        av_frame = nullptr;
      }
      if (codec_ctx) {
        avcodec_close(codec_ctx);
        codec_ctx = nullptr;
      }
      if (fmt_ctx) {
        avformat_free_context(fmt_ctx);
        fmt_ctx = nullptr;
      }
      if (io_ctx) {
        av_free(io_ctx->buffer);
        av_free(io_ctx);
        io_ctx = nullptr;
      }
      if (swr_ctx) {
        swr_free(&swr_ctx);
        swr_ctx = nullptr;
      }
    }

    bool AppData::set(std::vector<uint8_t> &memory, std::string *error) {
      data = std::move(memory);
      resetState();

      // open video
      fmt_ctx = avformat_alloc_context();
      io_ctx = avio_alloc_context((unsigned char *)av_malloc(kBufferSize), kBufferSize, 0, this, bufferRead, NULL, bufferSeek);
      fmt_ctx->pb = io_ctx;
      if (avformat_open_input(&fmt_ctx, "memory input", NULL, NULL) != 0) {
        if (error) {
          *error = "failed to open input";
        }
        return false;
      }

      // find stream info
      if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        if (error) {
          *error = "failed to get stream info";
        }
        return false;
      }

      // dump debug info
      // av_dump_format(fmt_ctx, 0, argv[1], 0);

       // find the video stream
      for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i)
      {
          if (fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
          {
              stream_idx = i;
              break;
          }
      }

      if (stream_idx == -1) {
        if (error) {
          *error = "failed to find stream";
        }
        return false;
      }

      audio_stream = fmt_ctx->streams[stream_idx];
      codec_ctx = audio_stream->codec;

      // find the decoder
      decoder = avcodec_find_decoder(codec_ctx->codec_id);
      if (decoder == NULL) {
        if (error) {
          *error = "failed to find decoder";
        }
        return false;
      }

      // open the decoder
      if (avcodec_open2(codec_ctx, decoder, NULL) < 0) {
        if (error) {
          *error = "failed to open codec";
        }
        return false;
      }

      // allocate the audio frames
      av_frame = av_frame_alloc();

      return true;
    }

    int AppData::bufferRead(void *opaque, unsigned char *buf, int buf_size) {
      AppData *appData = (AppData *)opaque;
      int64_t readLength = std::min<int64_t>(buf_size, appData->data.size() - appData->dataPos);
      if (readLength > 0) {
        memcpy(buf, appData->data.data() + appData->dataPos, readLength);
        appData->dataPos += readLength;
        return readLength;
      } else {
        return AVERROR_EOF;
      }
    }

    /**
     * Tell ffmpeg how to seek through the buffer.
     */
    int64_t AppData::bufferSeek(void *opaque, int64_t offset, int whence) {
      AppData *appData = (AppData *)opaque;
      if (whence == AVSEEK_SIZE) {
        return appData->data.size();
      } else {
        int64_t newPos;
        if (whence == SEEK_SET) {
          newPos = offset;
        } else if (whence == SEEK_CUR) {
          newPos = appData->dataPos + offset;
        } else if (whence == SEEK_END) {
          newPos = appData->data.size() + offset;
        } else {
          newPos = offset;
        }

        newPos = std::min<int64_t>(std::max<int64_t>(newPos, 0), appData->data.size());
        appData->dataPos = newPos;
        return newPos;
      }
    }

    PlanesVector AppData::read(std::string *error) {
      size_t numChannels = codec_ctx->channels;
      PlanesVector planes(numChannels);
      for (size_t i = 0; i < numChannels; i++) {
        planes[i].reserve(8 * 1024 / sizeof(float));
      }

      for (;;) {
        bool packetOk = false;
        bool packetValid = false;
        while (!packetValid || packet.stream_index != stream_idx) {
          if (packetValid) {
            av_free_packet(&packet);
            packetValid = false;
          }

          int ret = av_read_frame(fmt_ctx, &packet);
          packetValid = true;
          if (ret == AVERROR_EOF) {
            break;
          } else if (ret < 0) {
            if (error) {
              *error = "packet read error";
            }
            av_free_packet(&packet);
            return PlanesVector();
          } else {
            packetOk = true;
            break;
          }
        }
        // we have a valid packet at this point
        if (packetOk) {
          int frame_finished = 0;

          if (avcodec_decode_audio4(codec_ctx, av_frame, &frame_finished, &packet) < 0) {
            if (error) {
              *error = "failed to decode packet";
            }
            av_free_packet(&packet);
            return PlanesVector();
          }

          if (frame_finished) {
            if (!swr_ctx) {
              // allocate the resampler
              int channel_layout = codec_ctx->channels == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
              swr_ctx = swr_alloc_set_opts(
                nullptr,
                channel_layout,
                AV_SAMPLE_FMT_FLTP,
                codec_ctx->sample_rate,
                channel_layout,
                codec_ctx->sample_fmt,
                codec_ctx->sample_rate,
                0,
                nullptr
              );
              swr_init(swr_ctx);
            }

            swr_convert(swr_ctx, sr_planes, av_frame->nb_samples, (const uint8_t **)av_frame->extended_data, av_frame->nb_samples);

            const size_t numSamples = av_frame->nb_samples;
            for (size_t i = 0; i < numChannels; i++) {
              Plane &plane = planes[i];
              plane.resize(plane.size() + numSamples);
              float *frameStart = plane.data() + plane.size() - numSamples;
              memcpy(frameStart, sr_planes[i], numSamples * sizeof(float));
            }

            av_free_packet(&packet);

            continue;
          } else {
            av_free_packet(&packet);

            continue;
          }
        } else {
          av_free_packet(&packet);

          if (!planes[0].size()) {
            *error = "stream had no samples";
          }

          return planes;
        }
      }
    }

    float AppData::getSampleRate() {
      return codec_ctx->sample_rate;
    }

    std::unique_ptr<lab::AudioBus> LoadInternal(std::vector<uint8_t> &buffer, bool mixToMono, std::string *error)
    {
        AppData appData;
        if (!appData.set(buffer, error)) return nullptr; // takes ownership
        auto planes = appData.read(error);

        const size_t numChannels = planes.size();
        if (!numChannels) return nullptr;

        const size_t numSamples = planes[0].size();
        if (!numSamples) return nullptr;

        const size_t busChannelCount = mixToMono ? 1 : numChannels;

        // Create AudioBus where we'll put the PCM audio data
        std::unique_ptr<lab::AudioBus> audioBus(new lab::AudioBus(busChannelCount, numSamples));
        audioBus->setSampleRate(appData.getSampleRate());

        // Deinterleave stereo into LabSound/WebAudio planar channel layout
        // nqr::DeinterleaveChannels(audioData->samples.data(), planarSamples.data(), numSamples, numChannels, numSamples);

        // Mix to mono if stereo -- easier to do in place instead of using libnyquist helper functions
        // because we've already deinterleaved
        if (numChannels == 2 && mixToMono)
        {
            float *destinationMono = audioBus->channel(0)->mutableData();
            float *leftSamples = planes[0].data();
            float *rightSamples = planes[1].data();

            for (size_t i = 0; i < numSamples; i++)
            {
                destinationMono[i] = 0.5f * (leftSamples[i] + rightSamples[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < busChannelCount; i++)
            {
                memcpy(audioBus->channel(i)->mutableData(), planes[i].data(), numSamples * sizeof(float));
            }
        }

        return audioBus;
    }
}

namespace lab
{

std::mutex g_fileIOMutex;

std::unique_ptr<AudioBus> MakeBusFromFile(const char *filePath, bool mixToMono, std::string *error)
{
    std::lock_guard<std::mutex> lock(g_fileIOMutex);
    std::ifstream inputStream(filePath, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type size = inputStream.tellg();
    if (size >= 0) {
      std::vector<unsigned char> buffer(size);
      inputStream.seekg(0, std::ios::beg);
      inputStream.read((char *)buffer.data(), size);
      return detail::LoadInternal(buffer, mixToMono, error);
    } else {
      return nullptr;
    }
}

std::unique_ptr<AudioBus> MakeBusFromMemory(std::vector<uint8_t> &buffer, bool mixToMono, std::string *error)
{
    return detail::LoadInternal(buffer, mixToMono, error);
}

std::unique_ptr<AudioBus> MakeBusFromRawBuffer(size_t sampleRate, size_t numChannels, size_t numSamples, float *frames[], bool mixToMono, std::string *error)
{
  const size_t busChannelCount = mixToMono ? 1 : numChannels;

  // Create AudioBus where we'll put the PCM audio data
  std::unique_ptr<lab::AudioBus> audioBus(new lab::AudioBus(busChannelCount, numSamples));
  audioBus->setSampleRate(sampleRate);

  // Deinterleave stereo into LabSound/WebAudio planar channel layout
  // nqr::DeinterleaveChannels(audioData->samples.data(), planarSamples.data(), numSamples, numChannels, numSamples);

  // Mix to mono if stereo -- easier to do in place instead of using libnyquist helper functions
  // because we've already deinterleaved
  if (numChannels == 2 && mixToMono)
  {
      float *destinationMono = audioBus->channel(0)->mutableData();
      float *leftSamples = frames[0];
      float *rightSamples = frames[1];

      for (size_t i = 0; i < numSamples; i++)
      {
          destinationMono[i] = 0.5f * (leftSamples[i] + rightSamples[i]);
      }
  }
  else
  {
      for (size_t i = 0; i < busChannelCount; i++)
      {
          memcpy(audioBus->channel(i)->mutableData(), frames[i], numSamples * sizeof(float));
      }
  }

  return audioBus;
}

} // end namespace lab
