#include <fstream>

#include "qret/algorithm/data/qrom.h"
#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/ir/json.h"

using qret::BigInt;

struct UncomputeQROMGen : public qret::frontend::CircuitGenerator {
    const std::vector<BigInt>& dict;
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "UncomputeQROM";
    explicit UncomputeQROMGen(
            qret::frontend::CircuitBuilder* builder,
            std::uint64_t address_size,
            std::uint64_t value_size,
            const std::vector<BigInt>& dict
    )
        : CircuitGenerator(builder)
        , address_size{address_size}
        , value_size{value_size}
        , dict{dict} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, address_size - 1, Attribute::CleanAncilla);
    }
    qret::frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto aux = GetQubits(2);

        const auto max_address = std::size_t{1} << address_size;

        qret::frontend::gate::QROMImm(address, value, aux, dict);
        qret::frontend::gate::UncomputeQROMImm(address, value, aux, dict);

        return EndCircuitDefinition();
    }
};

int main() {
    using qret::frontend::gate::MultiControlledQROMImmGen;
    const auto address_size = 5;
    const auto value_size = 6;
    const auto max_address = std::size_t{1} << address_size;
    const auto max_value = BigInt{1} << value_size;
    const auto get_value = [max_value](const auto x) -> BigInt {
        return (x * BigInt{12323474} + 234956789) % max_value;
    };
    auto dict = std::vector<BigInt>();
    dict.reserve(std::size_t{1} << address_size);
    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        dict.emplace_back(get_value(address_value));
    }

    // create circuit
    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("uncompute_qrom", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto gen = UncomputeQROMGen(&builder, address_size, value_size, dict);
    gen.Generate();

    // serialization
    auto j = qret::Json();
    j = *builder.GetModule();

    // write json to file
    auto ofs = std::ofstream("uncompute_qrom.json");
    ofs << std::setw(4) << j;

    return 0;
}
