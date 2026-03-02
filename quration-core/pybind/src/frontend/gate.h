#pragma once

#include <nanobind/nanobind.h>

namespace pyqret {
void BindIntrinsicGate(nanobind::module_& m);
void BindControlFlow(nanobind::module_& m);
}  // namespace pyqret
