/**
 * Implementation file for AudioContext bindings.
 */

#include "audio_context.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

void register_audio_context(nb::module_ &m) {
    // Bind the AudioContext class
    nb::class_<lab::AudioContext, std::shared_ptr<lab::AudioContext>>(m, "_AudioContext")
        .def(nb::init<float, unsigned int, unsigned int>(),
             nb::arg("sample_rate") = 44100.0f,
             nb::arg("channels") = 2,
             nb::arg("frames_per_buffer") = 256)
        .def("start", &lab::AudioContext::start)
        .def("stop", &lab::AudioContext::stop)
        .def("current_time", &lab::AudioContext::currentTime)
        .def("destination", &lab::AudioContext::destination)
        .def("connect", [](lab::AudioContext& ctx, 
                          std::shared_ptr<lab::AudioNode> source, 
                          std::shared_ptr<lab::AudioNode> destination,
                          unsigned int output = 0, 
                          unsigned int input = 0) {
            ctx.connect(source, destination, output, input);
        }, nb::arg("source"), nb::arg("destination"), nb::arg("output") = 0, nb::arg("input") = 0)
        .def("disconnect", [](lab::AudioContext& ctx, std::shared_ptr<lab::AudioNode> node) {
            ctx.disconnect(node);
        }, nb::arg("node"))
        .def("create_oscillator", [](lab::AudioContext& ctx) {
            return ctx.createOscillator();
        })
        .def("create_gain", [](lab::AudioContext& ctx) {
            return ctx.createGain();
        })
        .def("create_biquad_filter", [](lab::AudioContext& ctx) {
            return ctx.createBiquadFilter();
        })
        .def("create_buffer_source", [](lab::AudioContext& ctx) {
            return ctx.createBufferSource();
        })
        .def("create_analyzer", [](lab::AudioContext& ctx) {
            return ctx.createAnalyser();
        })
        .def("create_function", [](lab::AudioContext& ctx, int channels) {
            // Note: This is a custom node that might need special handling
            // depending on how LabSound implements function nodes
            return ctx.createFunctionNode(channels);
        }, nb::arg("channels") = 1)
        .def("create_channel_merger", [](lab::AudioContext& ctx, int inputs) {
            return ctx.createChannelMerger(inputs);
        }, nb::arg("inputs") = 2)
        .def("create_channel_splitter", [](lab::AudioContext& ctx, int outputs) {
            return ctx.createChannelSplitter(outputs);
        }, nb::arg("outputs") = 2)
        .def("create_constant_source", [](lab::AudioContext& ctx) {
            return ctx.createConstantSource();
        })
        .def("create_convolver", [](lab::AudioContext& ctx) {
            return ctx.createConvolver();
        })
        .def("create_delay", [](lab::AudioContext& ctx, float maxDelayTime) {
            return ctx.createDelay(maxDelayTime);
        }, nb::arg("max_delay_time") = 2.0f)
        .def("create_dynamics_compressor", [](lab::AudioContext& ctx) {
            return ctx.createDynamicsCompressor();
        })
        .def("create_panner", [](lab::AudioContext& ctx) {
            return ctx.createPanner();
        })
        .def("create_stereo_panner", [](lab::AudioContext& ctx) {
            return ctx.createStereoPanner();
        })
        .def("create_wave_shaper", [](lab::AudioContext& ctx) {
            return ctx.createWaveShaper();
        });
}
