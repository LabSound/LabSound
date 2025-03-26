/**
 * Header file for AudioContext bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the AudioContext class with nanobind.
 */
void register_audio_context(nb::module_ &m);
