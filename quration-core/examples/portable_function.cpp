#include "qret/base/portable_function.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>

using namespace qret;

int main() {
    auto builder = PortableFunctionBuilder{};
    auto x = builder.AddIntInputVariable("x");
    builder.AddIntOutputVariable("res") = builder.Mux(x % 2 == 0, x / 2, 3 * x + 1);
    auto function = builder.Compile("collatz");

    const auto seed = std::uint64_t{0};
    auto engine = std::mt19937_64{seed};
    for (int i = 0; i < 10; i++) {
        auto input = std::abs(static_cast<std::int64_t>(engine()));
        auto test = function.Run({{"x", input}}).GetInt("res");
        auto expected = input % 2 == 0 ? input / 2 : 3 * input + 1;
        std::cout << input << ' ' << test << ' ' << expected << std::endl;
        assert(test == expected);
    }
}
