#include <nanobind/nanobind.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <fmt/format.h>

#include "qret/base/type.h"
#include "qret/math/integer.h"
#include "qret/math/lcu_helper.h"
#include "qret/version.h"

// bindings of qret
#include "backend/backend.h"
#include "common.h"
#include "frontend/frontend.h"
#if defined(QRET_BUILD_ALGORITHM)
#include "algorithm/algorithm.h"
#endif
#include "runtime/runtime.h"

namespace nb = nanobind;

namespace {
auto NBintToBigInt(const nb::int_& x) {
    return qret::BigInt(nb::str(x).c_str());
};
}  // namespace

void BindBase(nb::module_& m) {
    using qret::BigInt;
    nb::class_<BigInt>(m, "BigInt")
            .def(nb::init_implicit<std::int64_t>())
            .def(nb::init_implicit<std::string>())
            .def("__init__",
                 [](BigInt* t, const nb::int_& x) { new (t) BigInt(nb::str(x).c_str()); })
            .def("to_str", [](const BigInt& x) { return x.str(); })
            .def("to_int", [](const BigInt& x) { return nb::int_(nb::str(x.str().c_str())); })
            .def("__add__", [](const BigInt& x, const BigInt& y) -> BigInt { return x + y; })
            .def("__iadd__", [](BigInt& x, const BigInt& y) -> BigInt& { return x += y; })
            .def("__sub__", [](const BigInt& x, const BigInt& y) -> BigInt { return x - y; })
            .def("__isub__", [](BigInt& x, const BigInt& y) -> BigInt& { return x -= y; })
            .def("__mul__", [](const BigInt& x, const BigInt& y) -> BigInt { return x * y; })
            .def("__imul__", [](BigInt& x, const BigInt& y) -> BigInt& { return x *= y; })
            .def("__eq__", [](const BigInt& x, const BigInt& y) -> bool { return x == y; })
            .def("__ne__", [](const BigInt& x, const BigInt& y) -> bool { return x != y; })
            .def("__str__", pyqret::BindFmtUsingOstream<BigInt>);
    nb::implicitly_convertible<nb::int_, BigInt>();
    m.def("bool_array_as_int",
          [](const std::vector<bool>& bool_array) { return qret::BoolArrayAsBigInt(bool_array); })
            .def("int_as_bool_array", [](const BigInt& x) { return qret::BigIntAsBoolArray(x); })
            .def("modular_multiplicative_inverse", &qret::math::ModularMultiplicativeInverse)
            .def("preprocess_lcu_coefficients_for_reversible_sampling",
                 &qret::math::PreprocessLCUCoefficientsForReversibleSampling);
}

NB_MODULE(_qret_impl, m) {
    m.attr("__version__") =
            fmt::format("{}.{}.{}", QRET_VERSION_MAJOR, QRET_VERSION_MINOR, QRET_VERSION_PATCH);
    m.def("get_major_version", []() {
         return QRET_VERSION_MAJOR;
     }).def("get_minor_version", []() {
           return QRET_VERSION_MINOR;
       }).def("get_patch_version", []() { return QRET_VERSION_PATCH; });

    BindBase(m);

    {
        auto m_frontend = m.def_submodule("frontend");
        pyqret::BindFrontend(m_frontend);
    }
#if defined(QRET_BUILD_ALGORITHM)
    {
        auto m_algorithm = m.def_submodule("algorithm");
        pyqret::BindAlgorithm(m_algorithm);
    }
#endif
    {
        auto m_runtime = m.def_submodule("runtime");
        pyqret::BindRuntime(m_runtime);
    }
    {
        auto m_backend = m.def_submodule("backend");
        pyqret::BindBackend(m_backend);
    }
}
