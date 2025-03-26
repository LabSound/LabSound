/**
 * Header file for PannerNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the PannerNode class with nanobind.
 */
void register_panner_node(nb::module_ &m);
