#include "qret/transforms/scalar/delete_consecutive_same_pauli.h"

#include <gtest/gtest.h>

#include "qret/base/cast.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"

using namespace qret;
using frontend::CircuitBuilder, frontend::CircuitGenerator;
using ir::BinaryInst;
using ir::DeleteConsecutiveSamePauli;
using ir::Opcode;
using ir::ParametrizedRotationInst;
using ir::TernaryInst;
using ir::UnaryInst;

struct TestRGen : public CircuitGenerator {
    static inline const char* Name = "TestR";
    explicit TestRGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("q", Type::Qubit, 3, Attribute::Operate);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto q = GetQubits(0);

        frontend::gate::X(q[0]);
        frontend::gate::Y(q[0]);  // 3
        frontend::gate::Z(q[0]);  // 1
        frontend::gate::Y(q[2]);  // 2
        frontend::gate::Y(q[0]);  // 0
        frontend::gate::Y(q[0]);  // 0
        frontend::gate::Z(q[0]);  // 1
        frontend::gate::Y(q[2]);  // 2
        frontend::gate::CX(q[1], q[2]);
        frontend::gate::Y(q[0]);  // 3
        frontend::gate::RX(q[0], 0);
        frontend::gate::X(q[0]);
        frontend::gate::CCY(q[0], q[1], q[2]);
        frontend::gate::X(q[0]);

        return EndCircuitDefinition();
    }
};
TEST(SinglePauli, IgnoreConsecutiveSamePauli) {
    // If we ignore global phase, RZ == R1
    static constexpr auto Eps = 0.0001;

    for (auto pauli : {'X', 'Y', 'Z', '1'}) {
        ir::IRContext context;
        auto* module = ir::Module::Create("delete_consecutive_same_pauli", context);
        auto builder = CircuitBuilder(module);
        auto gen = TestRGen(&builder);
        auto* circuit = gen.Generate();

        auto* ir = circuit->GetIR();
        DeleteConsecutiveSamePauli().RunOnFunction(*ir);

        ASSERT_EQ(ir->GetNumBBs(), 1);
        const auto* bb = ir->GetEntryBB();
        auto itr = bb->begin();
        ASSERT_EQ(itr->GetOpcode().GetCode(), Opcode::Table::X);
        ASSERT_EQ(Cast<UnaryInst>(itr.GetPointer())->GetQubit().id, 0);
        ++itr;
        ASSERT_EQ(itr->GetOpcode().GetCode(), Opcode::Table::CX);
        ASSERT_EQ(Cast<BinaryInst>(itr.GetPointer())->GetQubit0().id, 1);
        ASSERT_EQ(Cast<BinaryInst>(itr.GetPointer())->GetQubit1().id, 2);
        ++itr;
        ASSERT_EQ(itr->GetOpcode().GetCode(), Opcode::Table::RX);
        ASSERT_EQ(Cast<ParametrizedRotationInst>(itr.GetPointer())->GetQubit().id, 0);
        ++itr;
        ASSERT_EQ(itr->GetOpcode().GetCode(), Opcode::Table::X);
        ASSERT_EQ(Cast<UnaryInst>(itr.GetPointer())->GetQubit().id, 0);
        ++itr;
        ASSERT_EQ(itr->GetOpcode().GetCode(), Opcode::Table::CCY);
        ASSERT_EQ(Cast<TernaryInst>(itr.GetPointer())->GetQubit0().id, 0);
        ASSERT_EQ(Cast<TernaryInst>(itr.GetPointer())->GetQubit1().id, 1);
        ASSERT_EQ(Cast<TernaryInst>(itr.GetPointer())->GetQubit2().id, 2);
        ++itr;
        ASSERT_EQ(itr->GetOpcode().GetCode(), Opcode::Table::X);
        ASSERT_EQ(Cast<UnaryInst>(itr.GetPointer())->GetQubit().id, 0);
        ++itr;
        ASSERT_EQ(itr->GetOpcode().GetCode(), Opcode::Table::Return);
        ++itr;
        ASSERT_EQ(itr, bb->end());
    }
}
