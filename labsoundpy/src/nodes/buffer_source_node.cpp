/**
 * Implementation file for AudioBufferSourceNode bindings.
 */

#include "nodes/buffer_source_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

void register_buffer_source_node(nb::module_ &m) {
    // Bind the AudioBufferSourceNode class (SampledAudioNode in LabSound)
    nb::class_<lab::SampledAudioNode, lab::AudioScheduledSourceNode, std::shared_ptr<lab::SampledAudioNode>>(m, "_AudioBufferSourceNode")
        .def("playback_rate", [](lab::SampledAudioNode& node) {
            return node.playbackRate();
        })
        .def("detune", [](lab::SampledAudioNode& node) {
            return node.detune();
        })
        .def("doppler_rate", [](lab::SampledAudioNode& node) {
            return node.dopplerRate();
        })
        .def("set_buffer", [](lab::SampledAudioNode& node, std::shared_ptr<lab::AudioBus> buffer) {
            node.setBus(buffer);
        }, nb::arg("buffer"))
        .def("get_buffer", [](lab::SampledAudioNode& node) {
            return node.bus();
        })
        .def("start", [](lab::SampledAudioNode& node, float when, float offset, float duration) {
            node.start(when, offset, duration);
        }, nb::arg("when") = 0.0f, nb::arg("offset") = 0.0f, nb::arg("duration") = 0.0f)
        .def("stop", [](lab::SampledAudioNode& node, float when) {
            node.stop(when);
        }, nb::arg("when") = 0.0f)
        .def("set_loop", [](lab::SampledAudioNode& node, bool loop, float loopStart, float loopEnd) {
            node.setLoop(loop, loopStart, loopEnd);
        }, nb::arg("loop"), nb::arg("loop_start") = 0.0f, nb::arg("loop_end") = 0.0f)
        .def("is_looping", &lab::SampledAudioNode::isLooping)
        .def("loop_start", &lab::SampledAudioNode::loopStart)
        .def("loop_end", &lab::SampledAudioNode::loopEnd)
        .def("is_playing", &lab::SampledAudioNode::isPlaying)
        .def("get_cursor", &lab::SampledAudioNode::getCursor);
}
