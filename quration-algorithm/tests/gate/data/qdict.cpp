#include "qret/algorithm/data/qdict.h"

#include <gtest/gtest.h>

#include "qret/algorithm/util/array.h"
#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitBuilder, runtime::SimulatorConfig, runtime::Simulator;

// Define test cases
class QDict : public testing::TestWithParam<std::tuple<std::size_t, std::size_t>> {};
INSTANTIATE_TEST_SUITE_P(
        LightCase,
        QDict,
        testing::Values(
                std::make_tuple(std::size_t{2}, std::size_t{2}),
                std::make_tuple(std::size_t{2}, std::size_t{5}),
                std::make_tuple(std::size_t{4}, std::size_t{10})
        )
);

//--------------------------------------------------//
// Test QDict
//--------------------------------------------------//
TEST_P(QDict, TestInitializeQDictUsingToffoliState) {
    using frontend::gate::InitializeQDictGen;
    const auto [address_size, value_size] = GetParam();
    const auto capacity = (std::size_t{1} << address_size) - 1;

    ir::IRContext context;
    auto* module = ir::Module::Create("qdict", context);
    auto builder = CircuitBuilder(module);
    auto gen = InitializeQDictGen(&builder, address_size, value_size, capacity);
    auto* circuit = gen.Generate();

    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, circuit->GetIR());
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    for (auto i = std::size_t{0}; i < capacity; ++i) {
        EXPECT_EQ(capacity, state.GetValue(address_size * i, address_size));
        EXPECT_EQ(0, state.GetValue(address_size * capacity + value_size * i, value_size));
    }
}
struct TestInjectIntoQDictGen : frontend::CircuitGenerator {
    std::size_t n;
    std::size_t m;
    std::size_t l;

    static inline const char* Name = "TestInjectIntoQDict";
    explicit TestInjectIntoQDictGen(
            CircuitBuilder* builder,
            std::size_t n,
            std::size_t m,
            std::size_t l
    )
        : CircuitGenerator(builder)
        , n{n}
        , m{m}
        , l{l} {}
    std::string GetName() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n * l, Attribute::Operate);
        arg.Add("value", Type::Qubit, m * l, Attribute::Operate);
        arg.Add("key", Type::Qubit, n, Attribute::Operate);
        arg.Add("input", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n + std::max({n, m, std::size_t{3}}), Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        using namespace frontend::gate;
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto key = GetQubits(2);
        auto input = GetQubits(3);
        auto aux = GetQubits(4);

        InitializeQDict(l, address, value);
        InjectIntoQDict(l, address, value, key, input, aux);

        return EndCircuitDefinition();
    }
};
TEST_P(QDict, TestInjectIntoQDictUsingToffoliState) {
    const auto [address_size, value_size] = GetParam();
    const auto capacity = (std::size_t{1} << address_size);
    const auto value_head = address_size * capacity;
    const auto key_head = value_head + value_size * capacity;
    const auto input_head = key_head + address_size;
    const auto max_value = BigInt{1} << value_size;
    const auto get_value = [max_value](const auto x) -> BigInt {
        return (x * BigInt{12323474} + 234956789) % max_value;
    };

    for (auto i = std::size_t{0}; i < capacity - 1; ++i) {
        ir::IRContext context;
        auto* module = ir::Module::Create("qdict", context);
        auto builder = CircuitBuilder(module);
        auto gen = TestInjectIntoQDictGen(&builder, address_size, value_size, capacity);
        auto* circuit = gen.Generate();

        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(key_head, address_size, i);
        simulator.SetValue(input_head, value_size, get_value(i));
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        for (auto j = std::size_t{0}; j < capacity; ++j) {
            const auto address = state.GetValue(address_size * j, address_size);
            if (address == capacity - 1) {
                EXPECT_EQ(0, state.GetValue(value_head + value_size * j, value_size));
            } else {
                EXPECT_EQ(
                        get_value(address),
                        state.GetValue(value_head + value_size * j, value_size)
                );
            }
        }
        EXPECT_EQ(i, state.GetValue(key_head, address_size));
        EXPECT_EQ(0, state.GetValue(input_head, value_size));
    }
}
struct TestExtractFromQDictGen : frontend::CircuitGenerator {
    std::size_t i_key;
    std::size_t n;
    std::size_t m;
    std::size_t l;

    static inline const char* Name = "TestExtractFromQDict";
    explicit TestExtractFromQDictGen(
            CircuitBuilder* builder,
            std::size_t i_key,
            std::size_t n,
            std::size_t m,
            std::size_t l
    )
        : CircuitGenerator(builder)
        , i_key{i_key}
        , n{n}
        , m{m}
        , l{l} {}
    std::string GetName() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n * l, Attribute::Operate);
        arg.Add("value", Type::Qubit, m * l, Attribute::Operate);
        arg.Add("key", Type::Qubit, n, Attribute::Operate);
        arg.Add("out", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n + std::max({n, m, std::size_t{3}}), Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        using namespace frontend::gate;
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto key = GetQubits(2);
        auto out = GetQubits(3);
        auto aux = GetQubits(4);

        const auto max_address = (BigInt{1} << n) - 1;
        const auto max_value = BigInt{1} << m;
        const auto get_value = [max_value](const auto x) -> BigInt {
            return (x * BigInt{12323474} + 234956789) % max_value;
        };

        InitializeQDict(l, address, value);
        // Write address and value manually
        for (auto i = std::size_t{0}; i < l - 1; ++i) {
            ApplyXToEachIfImm(max_address - i, address.Range(n * i, n));
            ApplyXToEachIfImm(get_value(i), value.Range(m * i, m));
        }
        ApplyXToEachIfImm(i_key, key);
        ExtractFromQDict(l, address, value, key, out, aux);

        return EndCircuitDefinition();
    }
};
TEST_P(QDict, TestExtractFromQDictUsingToffoliState) {
    const auto [address_size, value_size] = GetParam();
    const auto capacity = (std::size_t{1} << address_size);
    const auto value_head = address_size * capacity;
    const auto key_head = value_head + value_size * capacity;
    const auto out_head = key_head + address_size;
    const auto max_value = BigInt{1} << value_size;
    const auto get_value = [max_value](const auto x) -> BigInt {
        return (x * BigInt{12323474} + 234956789) % max_value;
    };

    for (auto i = std::size_t{0}; i < capacity - 1; ++i) {
        ir::IRContext context;
        auto* module = ir::Module::Create("qdict", context);
        auto builder = CircuitBuilder(module);
        auto gen = TestExtractFromQDictGen(&builder, i, address_size, value_size, capacity);
        auto* circuit = gen.Generate();

        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        for (auto j = std::size_t{0}; j < capacity; ++j) {
            const auto address = state.GetValue(address_size * j, address_size);
            if (address == capacity - 1) {
                EXPECT_EQ(0, state.GetValue(value_head + value_size * j, value_size));
            } else {
                EXPECT_EQ(
                        get_value(address),
                        state.GetValue(value_head + value_size * j, value_size)
                );
            }
        }
        EXPECT_EQ(i, state.GetValue(key_head, address_size));
        EXPECT_EQ(get_value(i), state.GetValue(out_head, value_size));
    }
}
struct TestQDictSequenceGen : frontend::CircuitGenerator {
    std::size_t key_size;
    std::size_t n;
    std::size_t m;
    std::size_t l;

    static inline const char* Name = "TestQDictSequence";
    explicit TestQDictSequenceGen(
            CircuitBuilder* builder,
            std::size_t key_size,
            std::size_t n,
            std::size_t m,
            std::size_t l
    )
        : CircuitGenerator(builder)
        , key_size{key_size}
        , n{n}
        , m{m}
        , l{l} {}
    std::string GetName() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n * l, Attribute::Operate);
        arg.Add("value", Type::Qubit, m * l, Attribute::Operate);
        arg.Add("key", Type::Qubit, n * key_size, Attribute::Operate);
        arg.Add("input", Type::Qubit, m * key_size, Attribute::Operate);
        arg.Add("out", Type::Qubit, m * key_size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, n + std::max({n, m, std::size_t{3}}), Attribute::CleanAncilla);
    }
    frontend::Circuit* Generate() const override {
        using namespace frontend::gate;
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto key = GetQubits(2);
        auto input = GetQubits(3);
        auto out = GetQubits(4);
        auto aux = GetQubits(5);

        InitializeQDict(l, address, value);
        for (auto i = std::size_t{0}; i < key_size; ++i) {
            InjectIntoQDict(l, address, value, key.Range(n * i, n), input.Range(m * i, m), aux);
        }
        for (auto i = std::size_t{0}; i < key_size; ++i) {
            ExtractFromQDict(l, address, value, key.Range(n * i, n), out.Range(m * i, m), aux);
        }

        return EndCircuitDefinition();
    }
};
TEST_P(QDict, TestQDictSequenceUsingToffoliState) {
    const auto [address_size, value_size] = GetParam();
    const auto capacity = (std::size_t{1} << address_size);
    // constexpr auto keys = std::array{1, 2, 0};
    constexpr auto keys = std::array{0};
    const auto key_size = keys.size();
    const auto value_head = address_size * capacity;
    const auto key_head = value_head + value_size * capacity;
    const auto input_head = key_head + address_size * key_size;
    const auto out_head = input_head + value_size * key_size;
    const auto max_value = BigInt{1} << value_size;
    const auto get_value = [max_value](const auto x) -> BigInt {
        return (x * BigInt{12323474} + 234956789) % max_value;
    };

    ir::IRContext context;
    auto* module = ir::Module::Create("qdict", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestQDictSequenceGen(&builder, key_size, address_size, value_size, capacity);
    auto* circuit = gen.Generate();

    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, circuit->GetIR());
    for (auto i = std::size_t{0}; i < key_size; ++i) {
        simulator.SetValue(key_head + address_size * i, address_size, keys[i]);
        simulator.SetValue(input_head + value_size * i, value_size, get_value(keys[i]));
    }
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    for (auto i = std::size_t{0}; i < key_size; ++i) {
        EXPECT_EQ(get_value(keys[i]), state.GetValue(out_head + value_size * i, value_size));
    }
}
