/**
 * Header file for DelayNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the DelayNode class with nanobind.
 */
void register_delay_node(nb::module_ &m);
