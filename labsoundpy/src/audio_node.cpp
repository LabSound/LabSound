/**
 * Implementation file for AudioNode bindings.
 */

#include "audio_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

void register_audio_node(nb::module_ &m) {
    // Bind the AudioNode base class
    nb::class_<lab::AudioNode, std::shared_ptr<lab::AudioNode>>(m, "_AudioNode")
        .def("connect", [](lab::AudioNode& node, 
                          std::shared_ptr<lab::AudioNode> destination,
                          unsigned int output = 0, 
                          unsigned int input = 0) {
            node.connect(output, destination, input);
        }, nb::arg("destination"), nb::arg("output") = 0, nb::arg("input") = 0)
        .def("disconnect", [](lab::AudioNode& node, unsigned int output = 0) {
            node.disconnect(output);
        }, nb::arg("output") = 0)
        .def("number_of_inputs", &lab::AudioNode::numberOfInputs)
        .def("number_of_outputs", &lab::AudioNode::numberOfOutputs)
        .def("channel_count", &lab::AudioNode::channelCount)
        .def("channel_count_mode", &lab::AudioNode::channelCountMode)
        .def("channel_interpretation", &lab::AudioNode::channelInterpretation)
        .def("set_channel_count", &lab::AudioNode::setChannelCount, nb::arg("count"))
        .def("set_channel_count_mode", &lab::AudioNode::setChannelCountMode, nb::arg("mode"))
        .def("set_channel_interpretation", &lab::AudioNode::setChannelInterpretation, nb::arg("interpretation"));
}
