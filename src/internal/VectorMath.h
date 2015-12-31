// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef VectorMath_h
#define VectorMath_h

// Defines the interface for several vector math functions whose implementation will ideally be optimized.

namespace lab {

namespace VectorMath {

// Vector scalar multiply and then add.
void vsma(const float* sourceP, int sourceStride, const float* scale, float* destP, int destStride, size_t framesToProcess);

void vsmul(const float* sourceP, int sourceStride, const float* scale, float* destP, int destStride, size_t framesToProcess);
void vadd(const float* source1P, int sourceStride1, const float* source2P, int sourceStride2, float* destP, int destStride, size_t framesToProcess);
void vintlve(const float* realSrcP, const float* imagSrcP, float* destP, size_t framesToProcess); // for KissFFT
void vdeintlve(const float* sourceP, float* realDestP, float* imagDestP, size_t framesToProcess); // for KissFFT

// Finds the maximum magnitude of a float vector.
void vmaxmgv(const float* sourceP, int sourceStride, float* maxP, size_t framesToProcess);

// Sums the squares of a float vector's elements.
void vsvesq(const float* sourceP, int sourceStride, float* sumP, size_t framesToProcess);

// For an element-by-element multiply of two float vectors.
void vmul(const float* source1P, int sourceStride1, const float* source2P, int sourceStride2, float* destP, int destStride, size_t framesToProcess);

// Multiplies two complex vectors.
void zvmul(const float* real1P, const float* imag1P, const float* real2P, const float* imag2P, float* realDestP, float* imagDestP, size_t framesToProcess);

// Copies elements while clipping values to the threshold inputs.
void vclip(const float* sourceP, int sourceStride, const float* lowThresholdP, const float* highThresholdP, float* destP, int destStride, size_t framesToProcess);

} // namespace VectorMath

} // namespace lab

#endif // VectorMath_h
