// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include <LabSound/LabSound.h>
#include <LabSound/extended/FFTFrame.h>
#include <LabSound/extended/VectorMath.h>
#include <vector>

using namespace lab;


void writeData(const char* path, const float* data, int stride, int samples) {
    FILE* f = fopen(path, "wb");
    if (f) {
        if (stride == 1) {
            fwrite(data, sizeof(float)*samples, 1, f);
            fclose(f);
        }
        else {
            std::vector<float> buff;
            buff.resize(samples / stride);
            for (int i = 0, c = 0; i < samples; i += stride, ++c) {
                buff[c] = data[i];
            }
            fwrite(data, sizeof(float)*buff.size(), 1, f);
            fclose(f);
        }
    }
}

std::shared_ptr<AudioBus> makeBus440()
{
    // Create AudioBus where we'll put the PCM audio data
    std::shared_ptr<lab::AudioBus> audioBus(new lab::AudioBus(1, 44100));
    audioBus->setSampleRate(44100.f);
    float* data = audioBus->channel(0)->mutableData();
    float* buff = data;
    // Generate a 440 Hz sine wave
    const float kTwoPi = 2 * 3.14159265358979323846f;
    const float kFrequency = 440.0f;
    const float kPhaseIncrement = kTwoPi * kFrequency / audioBus->sampleRate();
    float phase = 0.0f;
    int c = audioBus->length();
    for (int i = 0; i < c; ++i)
    {
        data[i] = sinf(phase);
        phase += kPhaseIncrement;
        printf("%f\n", data[i]);
    }
    return audioBus;
}

// make a DC signal where channel 1 has 1.0, 2 has 2.0, 3 has 4.0, 4 has 8.0, etc
std::shared_ptr<AudioBus> makeBusSixChannel()
{
    std::shared_ptr<lab::AudioBus> audioBus(new lab::AudioBus(6, 44100));
    audioBus->setSampleRate(44100.f);
    float val = 1.f;
    int len = audioBus->length();
    for (int c = 0; c < 6; c++, val *= 2) {
        float* data = audioBus->channel(c)->mutableData();
        for (int i = 0; i < len; ++i)
            data[i] = val;
    }
    return audioBus;
}

class AudioDevice_UnitTest : public AudioDevice
{
public:
    std::unique_ptr<AudioBus> renderBus;
    std::unique_ptr<AudioBus> inputBus;
    int remainder = 0;

    explicit AudioDevice_UnitTest(
            const AudioStreamConfig & inputConfig,
            const AudioStreamConfig & outputConfig);
    virtual ~AudioDevice_UnitTest() {}

    float authoritativeDeviceSampleRateAtRuntime {44100.f};

    // AudioDevice Interface
    void render(AudioContext*,
                AudioSourceProvider*, int numberOfFrames, void * outputBuffer, void * inputBuffer,
                float lowThreshold = -1.f, float highThreshold = 1.f);
    virtual void start() override final {}
    virtual void stop() override final {}
    virtual bool isRunning() const override final {}
    virtual void backendReinitialize() override final {
        remainder = 0;
    }
};

AudioDevice_UnitTest::AudioDevice_UnitTest(
            const AudioStreamConfig & inputConfig,
            const AudioStreamConfig & outputConfig)
: AudioDevice(inputConfig, outputConfig)
{
    if (outputConfig.device_index >= 0) {
        renderBus.reset(new AudioBus(outputConfig.desired_channels, AudioNode::ProcessingSizeInFrames));
        renderBus->setSampleRate(authoritativeDeviceSampleRateAtRuntime);
    }
    if (inputConfig.device_index >= 0) {
        inputBus.reset(new AudioBus(inputConfig.desired_channels, AudioNode::ProcessingSizeInFrames));
        inputBus->setSampleRate(authoritativeDeviceSampleRateAtRuntime);
    }
}


// Pulls on our provider to get rendered audio stream.
void AudioDevice_UnitTest::render(AudioContext* context,
                                  AudioSourceProvider*, int numberOfFrames_,
                                  void * outputBuffer, void * inputBuffer,
                                  float lowThreshold, float highThreshold)
{
    float kLowThreshold = lowThreshold;
    float kHighThreshold = highThreshold;
    int numberOfFrames = numberOfFrames_;

    float* pIn = static_cast<float *>(inputBuffer);
    float* pOut = static_cast<float *>(outputBuffer);

    int in_channels = inputBus ? inputBus->numberOfChannels() : 0;
    int out_channels = renderBus->numberOfChannels();

    //if (pIn && numberOfFrames * in_channels)
    //    _ring->write(pIn, numberOfFrames * in_channels);
    
    auto interface = context->audioContextInterface().lock();
    
    auto copy = [&](int sampleCount, float* pOut){
        // this loop leverages the vclip operation to copy, interleave, and clip in one pass.
        for (int i = 0; i < out_channels; ++i)
        {
            int src_stride = 1;  // de-interleaved
            int dst_stride = out_channels;  // interleaved
            AudioChannel * channel = renderBus->channel(i);
                        
            VectorMath::vclip(channel->data(), src_stride,
                              &kLowThreshold, &kHighThreshold,
                              pOut + i, dst_stride, sampleCount);
        }
    };

    if (remainder > 0) {
        // there might have been rendered frames left over from the previous render call
        copy(remainder, pOut);
        pOut += out_channels * remainder;
        numberOfFrames -= remainder; // reduce frames to render by old remainder
        remainder = 0;
    }

    // burn down the numberOfFrames
    while (numberOfFrames > AudioNode::ProcessingSizeInFrames)
    {
        if (in_channels)
        {
            /*
            _ring->read(_scratch, in_channels * kRenderQuantum);
            for (int i = 0; i < in_channels; ++i)
            {
                int src_stride = in_channels;  // interleaved
                int dst_stride = 1;  // de-interleaved
                AudioChannel * channel = _inputBus->channel(i);
                VectorMath::vclip(_scratch + i, src_stride,
                                  &kLowThreshold, &kHighThreshold,
                                  channel->mutableData(), dst_stride, kRenderQuantum);
            }
            */
        }

        // generate new data
        _destinationNode->render(context,
                                 sourceProvider(),
                                 inputBus.get(), renderBus.get());
        interface->updateSamplingInfo();
        copy(AudioNode::ProcessingSizeInFrames, pOut);
        pOut += out_channels * AudioNode::ProcessingSizeInFrames;
        numberOfFrames -= AudioNode::ProcessingSizeInFrames;
    }
    remainder = numberOfFrames;
}

void verifySame(char const*const title,
                float const* b1, int stride1,
                float const* b2, int stride2,
                int offset, int len)
{
    float const* p1 = b1 + (stride1 * offset);
    float const* p2 = b2 + (stride2 * offset);
    for ( ; len > 0; --len, p1 += stride1, p2 += stride2) {
        if (*p1 != *p2) {
            printf("%s: differ\n", title);
            return;
        }
    }
    printf("%s: same\n", title);
}

void verifySpectrum(char const*const title, float const* b1, float const* b2, int offset, int len)
{
    float const* p1 = b1 + offset;
    float const* p2 = b2 + offset;
    
    FFTFrame* fft1 = new FFTFrame(len);
    fft1->computeForwardFFT(b1 + offset);
    FFTFrame* fft2 = new FFTFrame(len);
    fft2->computeForwardFFT(b2 + offset);

    if (!memcmp(fft1->realData(), fft2->realData(), len * sizeof(float)))
        return;
    printf("%s: differ\n", title);
}

void verifyNoInputs(char const*const title, AudioNode* n)
{
    if (n->numberOfInputs() != 0) {
        printf("%s: node %s is not disconnected\n", title, n->name());
        return;
    }
    
    printf("%s: node %s is properly disconnected\n", title, n->name());
}


int main(int argc, char *argv[]) try
{
    AudioStreamConfig outputConfig;
    outputConfig.device_index = 0;
    outputConfig.desired_channels = 2;
    outputConfig.desired_samplerate = 44100;
    AudioStreamConfig inputConfig;
    inputConfig.device_index = -1;
    inputConfig.desired_channels = 0;
    inputConfig.desired_samplerate = 0;

    std::shared_ptr<AudioDevice_UnitTest> device(new AudioDevice_UnitTest(inputConfig, outputConfig));
    auto context = std::make_shared<lab::AudioContext>(device, false, true);
    auto destinationNode = std::make_shared<AudioDestinationNode>(*context.get(), device);
    destinationNode->setChannelCount(outputConfig.desired_channels);
    device->setDestinationNode(destinationNode);
    context->setDestinationNode(destinationNode);
    device->setContext(context);
 
    // need to hold the bus til the end of the test
    auto bus440 = makeBus440();
    
    auto& ac = *context.get();
    auto san = std::shared_ptr<SampledAudioNode>(new SampledAudioNode(ac));
    san->setBus(bus440);
    san->schedule(0);
    ac.connect(ac.destinationNode(), san, 0, 0);

    const int output_length = 44100;
    std::vector<float> audio;
    audio.resize(output_length * 2);
    device->render(context.get(), nullptr, (int32_t) output_length, audio.data(), nullptr);
    
    verifySame("440", audio.data(), 2, bus440->channel(0)->data(), 1, 1000, 8192);
    writeData("/var/tmp/440reference.dat", bus440->channel(0)->data(), 1, 44100);
    writeData("/var/tmp/440rendered.dat", audio.data(), 2, 44100);
    ac.backendReinitialize();

    // reset the waveform to the beginning
    san->setBus(bus440);
    {
        ContextRenderLock r(context.get(), "schedule");
        san->printSchedule(r);
    }
    san->stop(0.f);
    {
        ContextRenderLock r(context.get(), "schedule");
        san->printSchedule(r);
    }
    san->schedule(0.25f);
    {
        ContextRenderLock r(context.get(), "schedule");
        san->printSchedule(r);
    }
    san->stop(0.75f);
    
    {
        ContextRenderLock r(context.get(), "schedule");
        san->printSchedule(r);
    }
    
    device->render(context.get(), nullptr, (int32_t) output_length, audio.data(), nullptr);
    float sample1 = *(audio.data() + 44100/4 - 1000); // a few samples less than 1/4 of second
    float sample2 = *(audio.data() + 44100/4 + 1000); // a few samples past 1/4 of second
    float sample3 = *(audio.data() + 3*44100/4 - 1000); // a few samples before 3/4 of second
    float sample4 = *(audio.data() + 3*44100/4 + 1000); // a few samples before 3/4 of second
    writeData("/var/tmp/foo2.dat", audio.data(), 2, output_length);
    ac.backendReinitialize();

    // sample1 and 4 should be 0, 2 and 3 should be non-zero.

#if 0
    verifySpectrum("440", audio.data(), bus440->channel(0)->data(), 1000, 1024);
#endif
    // disconnect has a bit of a tail to prevent popping, so render frames cycles
    ac.disconnect(ac.destinationNode());
    device->render(context.get(), nullptr, (int32_t) 8000, audio.data(), nullptr);
    verifyNoInputs("dest", ac.destinationNode().get());
    
    auto bus6 = makeBusSixChannel();
    san->setBus(bus6);

    // connect event channels to left, odd to right
    // left will contain 21, right 42
    ac.connect(ac.destinationNode(), san, 0, 0);  // 1
    ac.connect(ac.destinationNode(), san, 0, 2);  // 4
    ac.connect(ac.destinationNode(), san, 0, 4);  // 16
    ac.connect(ac.destinationNode(), san, 1, 1);  // 2
    ac.connect(ac.destinationNode(), san, 1, 3);  // 8
    ac.connect(ac.destinationNode(), san, 1, 5);  // 32
    san->schedule(0);
    device->render(context.get(), nullptr, (int32_t) 8000, audio.data(), nullptr, 0.f, 100.f);
    float test1 = *(audio.data() + 1000);
    float test2 = *(audio.data() + 1001);
    if (test1 != 21 || test2 != 42) {
        printf("failed bus mapping\n");
    }
    else {
        printf("passed bus mapping\n");
    }
    ac.backendReinitialize();


    return EXIT_SUCCESS;
} 
catch (const std::exception & e) 
{
    std::cerr << "unhandled fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
