#include <fmt/core.h>

#include <fstream>

#include "qret/algorithm/arithmetic/integer.h"
#include "qret/frontend/builder.h"
#include "qret/ir/json.h"

int main() {
    using qret::frontend::gate::AddCuccaroGen;
    const auto size = std::size_t{5};
    const auto dst_value = 10;
    const auto src_value = 17;

    // create circuit
    qret::ir::IRContext context;
    auto* module = qret::ir::Module::Create("cuccaro", context);
    auto builder = qret::frontend::CircuitBuilder(module);
    auto gen = AddCuccaroGen(&builder, size);
    gen.Generate();

    // serialization
    auto j = qret::Json();
    j = *builder.GetModule();

    // write json to file
    auto ofs = std::ofstream(fmt::format("add_cuccaro_{}.json", size));
    ofs << std::setw(4) << j;

    return 0;
}
