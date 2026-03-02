#include <fstream>

#include "qret/algorithm/data/qrom.h"
#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/ir/json.h"

int main() {
    using qret::BigInt, qret::frontend::gate::MultiControlledQROMImmGen;
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
    auto* module = qret::ir::Module::Create("qrom", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto gen = MultiControlledQROMImmGen(&builder, 0, address_size, value_size, dict);
    gen.Generate();

    // serialization
    auto j = qret::Json();
    j = *builder.GetModule();

    // write json to file
    auto ofs = std::ofstream("qrom.json");
    ofs << std::setw(4) << j;

    return 0;
}
