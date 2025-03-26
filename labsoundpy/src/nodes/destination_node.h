/**
 * Header file for AudioDestinationNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the AudioDestinationNode class with nanobind.
 */
void register_destination_node(nb::module_ &m);
