#include "frontend/transforms/inliner.h"

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <cassert>

#include "qret/ir/instructions.h"
#include "qret/transforms/ipo/inliner.h"

namespace pyqret {
void BindInliner(nanobind::module_& m) {
    m.def("inline_call_inst", [](qret::ir::CallInst* call, bool merge_trivial) {
        return qret::ir::InlineCallInst(call, merge_trivial);
    });
    m.def("inline_circuit", [](qret::ir::Function& func, std::int32_t depth, bool merge_trivial) {
        auto changed = false;
        if (depth < 0) {
            auto pass = qret::ir::RecursiveInlinerPass();
            pass.merge_trivial = merge_trivial;
            changed |= pass.RunOnFunction(func);
        } else {
            auto pass = qret::ir::InlinerPass();
            pass.merge_trivial = merge_trivial;
            for (auto i = std::int32_t{0}; i < depth; ++i) {
                changed |= pass.RunOnFunction(func);
            }
        }
        return changed;
    });
}
}  // namespace pyqret
