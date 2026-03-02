/**
 * @file qret/algorithm/util/multi_control.h
 */

#ifndef QRET_GATE_UTIL_MULTI_CONTROL_H
#define QRET_GATE_UTIL_MULTI_CONTROL_H

#include "qret/frontend/argument.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/instructions.h"
#include "qret/qret_export.h"

namespace qret::frontend::gate {
//------------------------------------------------//
// Functions
//------------------------------------------------//

/**
 * @brief Performs multi-controlled multi-not.
 * @details Calculate pauli X to all `t` if all qubits of `c` is |1>.
 * See fig25 of https://arxiv.org/abs/1706.07884 for more information.
 *
 * Switching algorithms depending on the number of target, control, aux qubits:
 *
 * 1. case1: c = 0                       --> just do X to all t
 * 2. case2: c = 1                       --> just do CX to all t, c[0]
 * 3. case3: c = 2                       --> fig24
 * 4. case4: c >= 3, (n-1)+m >= c-2      --> fig25
 * 5. case5: c >= 3, c-2 > (n-1)+m >= 1  --> See
 * https://algassert.com/circuits/2015/06/05/Constructing-Large-Controlled-Nots.html
 *
 * @param cs  size = c
 * @param tgt size = n
 * @param aux size = m (if n=1 and c>=3 then m>=1)
 */
QRET_EXPORT ir::CallInst* MCMX(const Qubits& cs, const Qubits& tgt, const DirtyAncillae& aux);

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

/**
 * @brief Implementation of MCMX.
 */
struct QRET_EXPORT MCMXGen : CircuitGenerator {
    std::size_t c;
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "MCMX";
    explicit MCMXGen(CircuitBuilder* builder, std::size_t c, std::size_t n, std ::size_t m)
        : CircuitGenerator(builder)
        , c{c}
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{},{})", GetName(), c, n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, c, Attribute::Operate);
        arg.Add("tgt", Type::Qubit, n, Attribute::Operate);
        arg.Add("aux", Type::Qubit, m, Attribute::DirtyAncilla);
    }
    Circuit* Generate() const override;
};
}  // namespace qret::frontend::gate

#endif  // QRET_GATE_UTIL_MULTI_CONTROL_H
