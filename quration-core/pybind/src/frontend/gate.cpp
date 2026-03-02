#include "frontend/gate.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/intrinsic.h"

namespace pyqret {
namespace nb = nanobind;
using namespace qret::frontend;

void BindIntrinsicGate(nb::module_& m) {
#define DEFINE_INTRINSIC(name, inst) def(name, gate::inst, nb::rv_policy::reference)
    m
            // Measurement gates
            .DEFINE_INTRINSIC("measure", Measure)
            // Unary gates
            .DEFINE_INTRINSIC("i", I)
            .DEFINE_INTRINSIC("x", X)
            .DEFINE_INTRINSIC("y", Y)
            .DEFINE_INTRINSIC("z", Z)
            .DEFINE_INTRINSIC("h", H)
            .DEFINE_INTRINSIC("s", S)
            .DEFINE_INTRINSIC("sdag", SDag)
            .DEFINE_INTRINSIC("t", T)
            .DEFINE_INTRINSIC("tdag", TDag)
            // Arbitrary rotation
            .DEFINE_INTRINSIC("rx", RX)
            .DEFINE_INTRINSIC("ry", RY)
            .DEFINE_INTRINSIC("rz", RZ)
            // Binary gates
            .DEFINE_INTRINSIC("cx", CX)
            .DEFINE_INTRINSIC("cy", CY)
            .DEFINE_INTRINSIC("cz", CZ)
            // Ternary gates
            .DEFINE_INTRINSIC("ccx", CCX)
            .DEFINE_INTRINSIC("ccy", CCY)
            .DEFINE_INTRINSIC("ccz", CCZ)
            // MultiControl gates
            .DEFINE_INTRINSIC("mcx", MCX)
            // Other gates
            .DEFINE_INTRINSIC("global_phase", GlobalPhase)
            // Classical
            .DEFINE_INTRINSIC("discrete_distribution", DiscreteDistribution);
}

void BindControlFlow(nb::module_& m) {
    m.def("qu_if", [](const Register& cond) { control_flow::If(cond); })
            .def("qu_else", control_flow::Else)
            .def("qu_end_if", control_flow::EndIf)
            .def("qu_switch", [](const Registers& value) { control_flow::Switch(value); })
            .def("qu_default", control_flow::Default)
            .def("qu_case", control_flow::Case)
            .def("qu_end_switch", control_flow::EndSwitch);
}
}  // namespace pyqret
