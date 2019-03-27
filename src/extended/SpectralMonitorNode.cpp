// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"
#include "LabSound/core/WindowFunctions.h"

#include "LabSound/extended/SpectralMonitorNode.h"

#include <ooura/fftsg.h>
#include <cmath>

namespace lab 
{

    using namespace lab;

    //////////////////////////////////////////////////////
    // Prviate FFT + SpectralMonitorNode Implementation //
    //////////////////////////////////////////////////////

    // FFT class directly inspired by that in Cinder Audio 2.
    struct FFT 
    {
    public:
        FFT(size_t size) : size(size)
        {
            oouraIp = (int *)calloc( 2 + (int)sqrt( size/2 ), sizeof( int ) );
            oouraW = (float *)calloc( size/2, sizeof( float ) );
        }

        ~FFT()
        {
            free(oouraIp);
            free(oouraW);
        }

        // does an in place transform of waveform to real and imag.
        // real values are on even, imag on odd
        void forward( std::vector<float>& waveform)
        {
            ooura::rdft( static_cast<int>(size), 1, &waveform[0], oouraIp, oouraW );
        }

        size_t size;
        int * oouraIp;
        float * oouraW;
    };

    class SpectralMonitorNode::SpectralMonitorNodeInternal 
    {
    public:

        SpectralMonitorNodeInternal() 
        : fft(0)
        , windowSize(std::make_shared<AudioSetting>("windowSize")) 
        {
            setWindowSize(512);
        }

        ~SpectralMonitorNodeInternal() 
        {
            delete fft;
        }

        void setWindowSize(int s) 
        {
            cursor = 0;
            windowSize->setUint32(static_cast<size_t>(s));

            buffer.resize(s);

            for (size_t i = 0; i < s; ++i) 
            {
                buffer[i] = 0;
            }

            delete fft;
            fft = new FFT(s);
        }
        
        float _db;

        size_t cursor;

        std::vector<float> buffer;
        std::recursive_mutex magMutex;

        std::shared_ptr<AudioSetting> windowSize;

        FFT * fft;
    };

    ////////////////////////////////
    // Public SpectralMonitorNode //
    ////////////////////////////////

    SpectralMonitorNode::SpectralMonitorNode() : AudioBasicInspectorNode(2)
    , internalNode(new SpectralMonitorNodeInternal())
    {
        m_settings.push_back(internalNode->windowSize);
        addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
        initialize();
    }

    SpectralMonitorNode::~SpectralMonitorNode()
    {
        uninitialize();
        delete internalNode;
    }

    void SpectralMonitorNode::process(ContextRenderLock& r, size_t framesToProcess)
    {
        // deal with the output in case the power monitor node is embedded in a signal chain for some reason.
        // It's merely a pass through though.

        AudioBus* outputBus = output(0)->bus(r);

        if (!isInitialized() || !input(0)->isConnected()) {
            if (outputBus)
                outputBus->zero();
            return;
        }

        AudioBus* bus = input(0)->bus(r);
        bool isBusGood = bus && bus->numberOfChannels() > 0 && bus->channel(0)->length() >= framesToProcess;
        if (!isBusGood) {
            outputBus->zero();
            return;
        }

        // specific to this node
        {
            std::vector<const float*> channels;
            size_t numberOfChannels = bus->numberOfChannels();
            for (size_t c = 0; c < numberOfChannels; ++ c)
                channels.push_back(bus->channel(c)->data());

            size_t sz = internalNode->windowSize->valueUint32();

            // if the fft is smaller than the quantum, just grab a chunk
            if (sz < framesToProcess) 
            {
                internalNode->cursor = 0;
                framesToProcess = internalNode->windowSize->valueUint32();
            }

            // if the quantum overlaps the end of the window, just fill up the buffer
            if (internalNode->cursor + framesToProcess > sz)
                framesToProcess = sz - internalNode->cursor;

            {
                std::lock_guard<std::recursive_mutex> lock(internalNode->magMutex);

                internalNode->buffer.resize(sz);

                for (size_t i = 0; i < framesToProcess; ++i) 
                {
                    internalNode->buffer[i + internalNode->cursor] = 0;
                }

                for (unsigned c = 0; c < numberOfChannels; ++c)
                {
                    for (size_t i = 0; i < framesToProcess; ++i) 
                    {
                        float p = channels[c][i];
                        internalNode->buffer[i + internalNode->cursor] += p;
                    }
                }

            }

            // advance the cursor
            internalNode->cursor += framesToProcess;
            if (internalNode->cursor >= internalNode->windowSize->valueUint32())
                internalNode->cursor = 0;
        }
        // to here

        // For in-place processing, our override of pullInputs() will just pass the audio data
        // through unchanged if the channel count matches from input to output
        // (resulting in inputBus == outputBus). Otherwise, do an up-mix to stereo.
        //
        if (bus != outputBus)
            outputBus->copyFrom(*bus);
    }

    void SpectralMonitorNode::reset(ContextRenderLock&)
    {
        internalNode->setWindowSize(internalNode->windowSize->valueUint32());
    }

    void SpectralMonitorNode::spectralMag(std::vector<float>& result) 
    {
        std::vector<float> window;

        {
            std::lock_guard<std::recursive_mutex> lock(internalNode->magMutex);
            window.swap(internalNode->buffer);
            internalNode->setWindowSize(internalNode->windowSize->valueUint32());
        }

        // http://www.ni.com/white-paper/4844/en/
        applyWindow(lab::window_blackman, window);
        internalNode->fft->forward(window);

        // similar to cinder audio2 Scope object, although Scope smooths spectral samples frame by frame
        // remove nyquist component - the first imaginary component
        window[1] = 0.0f;

        // compute normalized magnitude spectrum
        // @tofix - break this into vector cartisian -> polar and then vector lowpass. skip lowpass if smoothing factor is very small
        const float kMagScale = 1.0f ;/// detail->windowSize;
        for(size_t i = 0; i < window.size(); i += 2)
        {
            float re = window[i];
            float im = window[i+1];
            window[i/2] = sqrt(re * re + im * im) * kMagScale;
        }

        result.swap(window);
    }

    void SpectralMonitorNode::windowSize(size_t ws) 
    {
        internalNode->setWindowSize(ws);
    }

    size_t SpectralMonitorNode::windowSize() const 
    {
        return internalNode->windowSize->valueUint32();
    }

} // namespace lab
