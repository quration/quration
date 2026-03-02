#include "backend/mf.h"

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <ranges>

#include "qret/codegen/machine_function.h"

namespace pyqret {
using namespace qret;
namespace nb = nanobind;

void BindMF(nanobind::module_& m) {
    nb::class_<MachineInstruction>(m, "MachineInstruction")
            .def("__str__", [](const MachineInstruction& inst) { return inst.ToString(); });

    nb::class_<MachineBasicBlock>(m, "MachineBasicBlock")
            .def("num_instructions",
                 [](const MachineBasicBlock& bb) { return bb.NumInstructions(); })
            .def(
                    "__iter__",
                    [](const MachineBasicBlock& bb) {
                        auto view = bb
                                | std::views::transform([](const auto& itr) -> MachineInstruction& {
                                        return *itr;
                                    });
                        return nb::make_iterator<nb::rv_policy::reference_internal>(
                                nb::type<MachineInstruction>(),
                                "MBBIterator",
                                std::ranges::begin(view),
                                std::ranges::end(view)
                        );
                    },
                    // Nurse = return value, Patient = bb
                    // "bb" should be kept alive at least until "return value" is
                    // freed by the garbage collector.
                    nb::keep_alive<0, 1>()
            );

    nb::class_<MachineFunction>(m, "MachineFunction")
            .def("num_bbs", [](const MachineFunction& f) { return f.NumBBs(); })
            .def(
                    "__iter__",
                    [](const MachineFunction& f) {
                        return nb::make_iterator<nb::rv_policy::reference_internal>(
                                nb::type<MachineBasicBlock>(),
                                "MFIterator",
                                f.begin(),
                                f.end()
                        );
                    },
                    // Nurse = return value, Patient = f
                    // "f" should be kept alive at least until "return value" is
                    // freed by the garbage collector.
                    nb::keep_alive<0, 1>()
            );
}

}  // namespace pyqret
