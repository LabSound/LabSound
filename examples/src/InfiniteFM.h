// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"

/*
     Adapted from this ChucK patch:

     SinOsc a => SinOsc b => ADSR chainADSR => Gain g => Gain feedbackGain => DelayL chainDelay => g => dac;

     1 => b.gain;
     1 => b.freq;

     0.25 => chainADSR.gain;

     while(true)
     {
         Std.rand2(1, 100000) => a.gain;
         Std.rand2(1, 400) => a.freq;

         chainADSR.set(Std.rand2(1,100)::ms, Std.rand2(1,100)::ms, 0, 0::ms);
         chainADSR.keyOn();

         Std.rand2(1, 500)::ms => now;
     }
 */

// @tofix - This doesn't match the expected sonic result of the ChucK patch. Presumably
// it is because LabSound oscillators are wavetable generated?

