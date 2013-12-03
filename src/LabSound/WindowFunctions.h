// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "Float32Array.h"
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>

#ifndef PI
	#define PI       3.14159265358979323846
#endif

#ifndef TWO_PI
	#define TWO_PI   6.28318530717958647693
#endif

#ifndef M_TWO_PI
	#define M_TWO_PI   6.28318530717958647693
#endif

namespace LabSound {

enum WindowType { 
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

void applyWindow(WindowType wType, RefPtr<Float32Array> &buffer) {

	int bSize = buffer->length();

	switch (wType) {

		case window_rectangle: {
			for (int i = 0; i< bSize; i++) {
				buffer->data()[i] *= 0.5;
			}
		}
		break;

		case window_hamming: {
			for (int i = 0; i< bSize; i++) {
				buffer->data()[i] *= 0.54 - 0.46 * cos(TWO_PI * i / ( bSize));
			}
		}
		break;

		case window_hanning: {
			for (int i = 0; i< bSize; i++) {
				buffer->data()[i] *= 0.5 - (0.5 * cos(TWO_PI * i / ( bSize)));
			}
		}
		break;

		case window_hanningz: {
			for (int i = 0; i< bSize; i++) {
				buffer->data()[i] *= 0.5 * (1.0 - cos(TWO_PI * i / ( bSize)));
			}
		}
		break;

		case window_blackman: {

			for (int i= 0; i < bSize; i++) {
				buffer->data()[i] *= 0.42 - 0.50 * cos(TWO_PI * i / (bSize - 1.0)) + 0.08 * cos(4.0 * TWO_PI * i / (bSize - 1.0));
			} 
		}
		break;

		case window_blackman_harris: {
			for (int i = 0; i < bSize; i++) {
				buffer->data()[i] *= 0.35875 - 0.48829 * cos(TWO_PI * i / (bSize - 1.0)) + 0.14128 * cos(2.0 * TWO_PI * i / (bSize - 1.0)) - 0.01168 * cos(3.0*TWO_PI*i/( bSize - 1.0));
			}
		}
		break;

		case window_gaussian: {
			float a, b, c = 0.5;
			int n;
			for (n = 0; n <  bSize; n++) {
				a = (n - c * (bSize - 1)) / (sqrt(c) * (bSize - 1));
				b = -c * sqrt(a);
				buffer->data()[n] *= exp(b);
			}
		}
		break;

		case window_welch: {
			for (int i = 0; i < bSize; i++) {
				buffer->data()[i] *= 1.0 - sqrt((2.0 * i - bSize) / (bSize + 1.0));
			}
		}
		break;

		case window_bartlett: {
			for (int i = 0; i < bSize; i++) {
			  buffer->data()[i] *= 1.0 - fabs(2.0 * (i / bSize) - 1.0);
			}
		}
		break; 

		case window_parzen: {
			for (int i = 0; i < bSize; i++) {
				buffer->data()[i] *= 1.0 - abs((2.0 * i - bSize) / ( bSize + 1.0));
			}
		}
		break;

	}

}

} // End namespace LabSound