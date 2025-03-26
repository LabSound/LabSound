/**
 * Header file for WaveShaperNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the WaveShaperNode class with nanobind.
 */
void register_wave_shaper_node(nb::module_ &m);
