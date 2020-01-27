// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#include <cmath>
#include <vector>

namespace lab
{

enum WindowType
{
    window_rectangle,
    window_hamming,
    window_hanning,
    window_hanningz,
    window_blackman,
    window_blackman_harris,
    window_gaussian,
    window_welch,
    window_bartlett,
    window_parzen
};

inline void applyWindow(WindowType wType, std::vector<float> & buffer)
{
    static const float WINDOW_PI = 3.14159265358979323846f;
    static const float WINDOW_TAU = 6.28318530717958647693f;

    const int bSize = static_cast<int>(buffer.size());

    switch (wType)
    {

        case window_rectangle:
        {
            for (int i = 0; i < bSize; i++)
            {
                buffer[i] *= 0.5;
            }
        }
        break;

        case window_hamming:
        {
            for (int i = 0; i < bSize; i++)
            {
                buffer[i] *= 0.54f - 0.46f * cos(WINDOW_TAU * i / (bSize));
            }
        }
        break;

        case window_hanning:
        {
            for (int i = 0; i < bSize; i++)
            {
                buffer[i] *= 0.5f - (0.5f * cos(WINDOW_TAU * i / (bSize)));
            }
        }
        break;

        case window_hanningz:
        {
            for (int i = 0; i < bSize; i++)
            {
                buffer[i] *= 0.5f * (1.0f - cos(WINDOW_TAU * i / (bSize)));
            }
        }
        break;

        case window_blackman:
        {
            for (int i = 0; i < bSize; i++)
            {
                buffer[i] *= 0.42f - 0.50f * cos(WINDOW_TAU * i / (bSize - 1.0f)) + 0.08f * cos(4.0f * WINDOW_TAU * i / (bSize - 1.0f));
            }
        }
        break;

        case window_blackman_harris:
        {
            for (int i = 0; i < bSize; i++)
            {
                buffer[i] *= 0.35875f - 0.48829f * cos(WINDOW_TAU * i / (bSize - 1.0f)) + 0.14128f * 
                    cos(2.0f * WINDOW_TAU * i / (bSize - 1.0f)) - 0.01168f * cos(3.0f * WINDOW_TAU * i / (bSize - 1.0f));
            }
        }
        break;

        case window_gaussian:
        {
            float a, b, c = 0.5;
            int n;
            for (n = 0; n < bSize; n++)
            {
                a = (n - c * (bSize - 1)) / (sqrt(c) * (bSize - 1));
                b = -c * sqrt(a);
                buffer[n] *= exp(b);
            }
        }
        break;

        case window_welch:
        {
            for (int i = 0; i < bSize; i++)
            {
                buffer[i] *= 1.0f - sqrt((2.0f * i - bSize) / (bSize + 1.0f));
            }
        }
        break;

        case window_bartlett:
        {
            for (int i = 0; i < bSize; i++)
            {
                buffer[i] *= 1.0f - abs(2.0f * (i / bSize) - 1.0f);
            }
        }
        break;

        case window_parzen:
        {
            for (int i = 0; i < bSize; i++)
            {
                buffer[i] *= 1.0f - abs((2.0f * i - bSize) / (bSize + 1.0f));
            }
        }
        break;
    }
}

}  // End namespace lab
