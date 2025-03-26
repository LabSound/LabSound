/**
 * Header file for ChannelMergerNode bindings.
 */

#pragma once

#include <nanobind/nanobind.h>
#include "LabSound/LabSound.h"

namespace nb = nanobind;

/**
 * Register the ChannelMergerNode class with nanobind.
 */
void register_channel_merger_node(nb::module_ &m);
