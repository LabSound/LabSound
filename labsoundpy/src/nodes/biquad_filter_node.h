/**
 * Header file for BiquadFilterNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the BiquadFilterNode class with nanobind.
 */
void register_biquad_filter_node(nb::module_ &m);
