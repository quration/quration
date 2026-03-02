/**
 * @file qret/algorithm/util/write_register.cpp
 */

#include "qret/algorithm/util/write_register.h"

#include <fmt/core.h>

#include "qret/base/portable_function.h"
#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"  // DO NOT DELETE
#include "qret/frontend/circuit_generator.h"  // DO NOT DELETE
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
namespace {
std::vector<ir::Register> GetRVec(const Registers& rs) {
    auto ret = std::vector<ir::Register>();
    ret.reserve(rs.Size());
    for (const auto& r : rs) {
        ret.emplace_back(r.GetId());
    }
    return ret;
}
}  // namespace

//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallCFInst* WriteRegisters(const BigInt& imm, const Registers& out) {
    const auto n = out.Size();

    auto function = PortableFunction(
            fmt::format("WriteRegisters({},{})", imm, n),
            {{"input", PortableFunction::RegisterType::BoolArray(0)}},
            {{"output", PortableFunction::RegisterType::BoolArray(n)}},
            {}
    );

    for (auto i = std::int64_t{0}; i < static_cast<std::int32_t>(n); ++i) {
        if (((imm >> i) & 1) == 1) {
            function.LNot({"output", i}, {"output", i});
        }
    }

    return ir::CallCFInst::Create(
            PortableAnnotatedFunction(function),
            {},
            GetRVec(out),
            out.GetBuilder()->GetInsertionPoint()
    );
}
}  // namespace qret::frontend::gate
