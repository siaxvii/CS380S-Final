#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <torch/extension.h>
#include "sandbox_layer.h"

namespace py = pybind11;

PYBIND11_MODULE(sandboxed_ffmpeg, m) {
    m.def("load_tensor_from_file", &load_tensor_from_file, py::arg());
}
