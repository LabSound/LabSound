/**
 * Implementation file for WaveShaperNode bindings.
 */

#include "nodes/wave_shaper_node.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/ndarray.h>

void register_wave_shaper_node(nb::module_ &m) {
    // First, register the OverSampleType enum
    nb::enum_<lab::OverSampleType>(m, "OverSampleType")
        .value("NONE", lab::OverSampleType::NONE)
        .value("2X", lab::OverSampleType::X2)
        .value("4X", lab::OverSampleType::X4);
    
    // Create a mapping from string to OverSampleType for Pythonic interface
    auto oversample_from_string = [](const std::string& type_str) {
        if (type_str == "none") return lab::OverSampleType::NONE;
        if (type_str == "2x") return lab::OverSampleType::X2;
        if (type_str == "4x") return lab::OverSampleType::X4;
        throw nb::value_error("Invalid oversample type: " + type_str);
    };
    
    auto oversample_to_string = [](lab::OverSampleType type) {
        switch (type) {
            case lab::OverSampleType::NONE: return "none";
            case lab::OverSampleType::X2: return "2x";
            case lab::OverSampleType::X4: return "4x";
            default: return "unknown";
        }
    };
    
    // Bind the WaveShaperNode class
    nb::class_<lab::WaveShaperNode, lab::AudioNode, std::shared_ptr<lab::WaveShaperNode>>(m, "_WaveShaperNode")
        .def("set_curve", [](lab::WaveShaperNode& node, nb::ndarray<nb::numpy, float, nb::shape<nb::any>> curve) {
            // Convert numpy array to std::vector
            std::vector<float> curve_vec(curve.data(), curve.data() + curve.size());
            node.setCurve(curve_vec.data(), curve_vec.size());
        }, nb::arg("curve"))
        .def("curve", [](lab::WaveShaperNode& node) {
            // Get the curve data
            const float* curve_data = node.curve();
            size_t curve_size = node.curveSize();
            
            // Create a numpy array from the data
            return nb::ndarray<nb::numpy, float, nb::shape<nb::any>>(
                curve_data, {curve_size}, {sizeof(float)}
            );
        })
        .def("set_oversample", &lab::WaveShaperNode::setOversample, nb::arg("oversample"))
        .def("oversample", &lab::WaveShaperNode::oversample)
        // Add Pythonic interface for oversample
        .def("set_oversample_string", [oversample_from_string](lab::WaveShaperNode& node, const std::string& type_str) {
            node.setOversample(oversample_from_string(type_str));
        }, nb::arg("oversample"))
        .def("oversample_string", [oversample_to_string](lab::WaveShaperNode& node) {
            return oversample_to_string(node.oversample());
        });
}
