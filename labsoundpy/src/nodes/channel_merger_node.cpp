/**
 * Implementation file for ChannelMergerNode bindings.
 */

#include "nodes/channel_merger_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

void register_channel_merger_node(nb::module_ &m) {
    // Bind the ChannelMergerNode class
    nb::class_<lab::ChannelMergerNode, lab::AudioNode, std::shared_ptr<lab::ChannelMergerNode>>(m, "_ChannelMergerNode")
        .def("add_inputs", &lab::ChannelMergerNode::addInputs, nb::arg("count"))
        .def("set_output_channel_count", &lab::ChannelMergerNode::setOutputChannelCount, nb::arg("count"));
}
