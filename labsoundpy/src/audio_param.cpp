/**
 * Implementation file for AudioParam bindings.
 */

#include "audio_param.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

void register_audio_param(nb::module_ &m) {
    // Bind the AudioParam class
    nb::class_<lab::AudioParam, std::shared_ptr<lab::AudioParam>>(m, "_AudioParam")
        .def("value", &lab::AudioParam::value)
        .def("set_value", &lab::AudioParam::setValue, nb::arg("value"))
        .def("default_value", &lab::AudioParam::defaultValue)
        .def("min_value", &lab::AudioParam::minValue)
        .def("max_value", &lab::AudioParam::maxValue)
        .def("set_value_at_time", &lab::AudioParam::setValueAtTime, 
             nb::arg("value"), nb::arg("time"))
        .def("linear_ramp_to_value_at_time", &lab::AudioParam::linearRampToValueAtTime, 
             nb::arg("value"), nb::arg("time"))
        .def("exponential_ramp_to_value_at_time", &lab::AudioParam::exponentialRampToValueAtTime, 
             nb::arg("value"), nb::arg("time"))
        .def("set_target_at_time", &lab::AudioParam::setTargetAtTime, 
             nb::arg("target"), nb::arg("time"), nb::arg("time_constant"))
        .def("cancel_scheduled_values", &lab::AudioParam::cancelScheduledValues, 
             nb::arg("start_time"));
}
