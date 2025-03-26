/**
 * Header file for StereoPannerNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the StereoPannerNode class with nanobind.
 */
void register_stereo_panner_node(nb::module_ &m);
