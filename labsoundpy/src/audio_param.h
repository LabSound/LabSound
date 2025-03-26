/**
 * Header file for AudioParam bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the AudioParam class with nanobind.
 */
void register_audio_param(nb::module_ &m);
