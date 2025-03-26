/**
 * Implementation file for AnalyzerNode bindings.
 */

#include "nodes/analyzer_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/ndarray.h>

void register_analyzer_node(nb::module_ &m) {
    // Bind the AnalyzerNode class
    nb::class_<lab::AnalyserNode, lab::AudioNode, std::shared_ptr<lab::AnalyserNode>>(m, "_AnalyzerNode")
        .def("set_fft_size", &lab::AnalyserNode::setFftSize, nb::arg("size"))
        .def("fft_size", &lab::AnalyserNode::fftSize)
        .def("frequency_bin_count", &lab::AnalyserNode::frequencyBinCount)
        .def("set_min_decibels", &lab::AnalyserNode::setMinDecibels, nb::arg("min_db"))
        .def("min_decibels", &lab::AnalyserNode::minDecibels)
        .def("set_max_decibels", &lab::AnalyserNode::setMaxDecibels, nb::arg("max_db"))
        .def("max_decibels", &lab::AnalyserNode::maxDecibels)
        .def("set_smoothing_time_constant", &lab::AnalyserNode::setSmoothingTimeConstant, nb::arg("time_constant"))
        .def("smoothing_time_constant", &lab::AnalyserNode::smoothingTimeConstant)
        .def("get_float_frequency_data", [](lab::AnalyserNode& node) {
            size_t size = node.frequencyBinCount();
            std::vector<float> data(size);
            node.getFloatFrequencyData(data.data());
            
            // Create a numpy array from the data
            return nb::ndarray<nb::numpy, float, nb::shape<nb::any>>(
                data.data(), {size}, {sizeof(float)}
            );
        })
        .def("get_byte_frequency_data", [](lab::AnalyserNode& node, bool resample = false) {
            size_t size = node.frequencyBinCount();
            std::vector<uint8_t> data(size);
            node.getByteFrequencyData(data.data(), resample);
            
            // Create a numpy array from the data
            return nb::ndarray<nb::numpy, uint8_t, nb::shape<nb::any>>(
                data.data(), {size}, {sizeof(uint8_t)}
            );
        }, nb::arg("resample") = false)
        .def("get_float_time_domain_data", [](lab::AnalyserNode& node) {
            size_t size = node.fftSize();
            std::vector<float> data(size);
            node.getFloatTimeDomainData(data.data());
            
            // Create a numpy array from the data
            return nb::ndarray<nb::numpy, float, nb::shape<nb::any>>(
                data.data(), {size}, {sizeof(float)}
            );
        })
        .def("get_byte_time_domain_data", [](lab::AnalyserNode& node) {
            size_t size = node.fftSize();
            std::vector<uint8_t> data(size);
            node.getByteTimeDomainData(data.data());
            
            // Create a numpy array from the data
            return nb::ndarray<nb::numpy, uint8_t, nb::shape<nb::any>>(
                data.data(), {size}, {sizeof(uint8_t)}
            );
        });
}
