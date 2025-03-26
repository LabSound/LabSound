/**
 * Header file for AudioNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the AudioNode class with nanobind.
 */
void register_audio_node(nb::module_ &m);
