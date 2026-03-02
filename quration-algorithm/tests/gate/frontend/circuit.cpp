#include "qret/frontend/circuit.h"

#include <gtest/gtest.h>

#include <stdexcept>

#include "qret/algorithm/arithmetic/integer.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"

using namespace qret;

struct SizeErrorGen : public frontend::CircuitGenerator {
    std::uint64_t size;

    static inline const char* Name = "SizeError";
    explicit SizeErrorGen(frontend::CircuitBuilder* builder, std::uint64_t size)
        : CircuitGenerator(builder)
        , size{size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("dst", Type::Qubit, size, Attribute::Operate);
        arg.Add("src", Type::Qubit, size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto dst = GetQubits(0);
        auto src = GetQubits(1);
        auto aux = GetQubit(2);

        frontend::gate::AddCuccaro(dst, src.Range(2, 4), aux);

        return EndCircuitDefinition();
    }
};
TEST(ArgumentCheck, SizeError) {
    ir::IRContext context;
    auto* module = ir::Module::Create("argument_check_size_error", context);
    auto builder = frontend::CircuitBuilder(module);
    auto gen = SizeErrorGen(&builder, 5);
    EXPECT_THROW(gen.Generate(), std::runtime_error);
}
