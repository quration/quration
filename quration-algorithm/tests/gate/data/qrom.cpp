#include "qret/algorithm/data/qrom.h"

#include <gtest/gtest.h>

#include "qret/algorithm/util/array.h"
#include "qret/algorithm/util/reset.h"
#include "qret/algorithm/util/write_register.h"
#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/runtime/simulator.h"

using namespace qret;
using frontend::CircuitGenerator, frontend::CircuitBuilder, runtime::SimulatorConfig,
        runtime::Simulator;

//--------------------------------------------------//
// Test UnitaryIterBegin, UnitaryIterStep, and
// UnitaryIterEnd
//--------------------------------------------------//
struct TestUnaryIterationGen : public CircuitGenerator {
    std::uint64_t address_size;
    std::uint64_t value_size;
    BigInt address_value;

    static inline const char* Name = "TestUnaryIteration";
    explicit TestUnaryIterationGen(
            CircuitBuilder* builder,
            std::uint64_t address_size,
            std::uint64_t value_size,
            const BigInt& address_value
    )
        : CircuitGenerator(builder)
        , address_size{address_size}
        , value_size{value_size}
        , address_value{address_value} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, address_size - 1, Attribute::CleanAncilla);
    }
    BigInt GetValue(const BigInt& x) const {
        const auto max_value = BigInt{1} << value_size;
        return (x * BigInt{12323474} + 234956789) % max_value;
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto aux = GetQubits(2);

        const auto max_address = BigInt{1} << address_size;

        frontend::gate::ApplyXToEachIfImm(address_value, address);
        frontend::gate::UnaryIterBegin(0, address, aux);
        for (auto i = BigInt(0); i < max_address - 1; ++i) {
            frontend::gate::ApplyCXToEachIfImm(GetValue(i), aux[0], value);
            frontend::gate::UnaryIterStep(i, address, aux);
        }
        frontend::gate::ApplyCXToEachIfImm(GetValue(max_address - 1), aux[0], value);
        frontend::gate::UnaryIterEnd(max_address - 1, address, aux);

        return EndCircuitDefinition();
    }
};
TEST(UnitaryIter, UsingToffoliState) {
    const auto address_size = 5;
    const auto value_size = 6;
    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto gen = TestUnaryIterationGen(&builder, address_size, value_size, address_value);
        auto* circuit = gen.Generate();

        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(gen.GetValue(address_value), state.GetValue(address_size, value_size));
    }
}
//--------------------------------------------------//
// Test MultiControlledUnitaryIterBegin,
// MultiControlledUnitaryIterStep, and
// MultiControlledUnitaryIterEnd
//--------------------------------------------------//
struct TestMultiControlledUnaryIterationGen : public CircuitGenerator {
    static constexpr auto ControlSize = std::uint64_t{2};
    bool ctrl;
    std::uint64_t address_size;
    std::uint64_t value_size;
    BigInt address_value;

    static inline const char* Name = "TestMultiControlledUnaryIteration";
    explicit TestMultiControlledUnaryIterationGen(
            CircuitBuilder* builder,
            bool ctrl,
            std::uint64_t address_size,
            std::uint64_t value_size,
            const BigInt& address_value
    )
        : CircuitGenerator(builder)
        , ctrl{ctrl}
        , address_size{address_size}
        , value_size{value_size}
        , address_value{address_value} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, ControlSize, Attribute::Operate);
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, ControlSize + address_size - 1, Attribute::CleanAncilla);
    }
    BigInt GetValue(const BigInt& x) const {
        const auto max_value = BigInt{1} << value_size;
        return (x * BigInt{12323474} + 234956789) % max_value;
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto cs = GetQubits(0);
        auto address = GetQubits(1);
        auto value = GetQubits(2);
        auto aux = GetQubits(3);

        const auto max_address = BigInt{1} << address_size;

        // Initialize
        if (ctrl) {
            for (const auto& c : cs) {
                frontend::gate::X(c);
            }
        }
        frontend::gate::ApplyXToEachIfImm(address_value, address);

        frontend::gate::MultiControlledUnaryIterBegin(0, cs, address, aux);
        for (auto i = BigInt(0); i < max_address - 1; ++i) {
            frontend::gate::ApplyCXToEachIfImm(GetValue(i), aux[0], value);
            frontend::gate::MultiControlledUnaryIterStep(i, cs, address, aux);
        }
        frontend::gate::ApplyCXToEachIfImm(GetValue(max_address - 1), aux[0], value);
        frontend::gate::MultiControlledUnaryIterEnd(max_address - 1, cs, address, aux);

        return EndCircuitDefinition();
    }
};
TEST(MultiControlledUnitaryIter, UsingToffoliStateCtrlTrue) {
    const auto address_size = 5;
    const auto value_size = 6;
    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto gen = TestMultiControlledUnaryIterationGen(
                &builder,
                true,
                address_size,
                value_size,
                address_value
        );
        auto* circuit = gen.Generate();

        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ((1 << gen.ControlSize) - 1, state.GetValue(0, gen.ControlSize));
        EXPECT_EQ(address_value, state.GetValue(gen.ControlSize, address_size));
        EXPECT_EQ(
                gen.GetValue(address_value),
                state.GetValue(gen.ControlSize + address_size, value_size)
        );
    }
}
TEST(MultiControlledUnitaryIter, UsingToffoliStateCtrlFalse) {
    const auto address_size = 5;
    const auto value_size = 6;
    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto gen = TestMultiControlledUnaryIterationGen(
                &builder,
                false,
                address_size,
                value_size,
                address_value
        );
        auto* circuit = gen.Generate();

        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(0, state.GetValue(0, gen.ControlSize));
        EXPECT_EQ(address_value, state.GetValue(gen.ControlSize, address_size));
        EXPECT_EQ(0, state.GetValue(gen.ControlSize + address_size, value_size));
    }
}
//--------------------------------------------------//
// Test QROM, MultiControlledQROM
//--------------------------------------------------//
struct TestMultiControlledQROMGen : public CircuitGenerator {
    static constexpr auto ControlSize = std::uint64_t{2};
    bool ctrl;
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "TestQROMGen";
    explicit TestMultiControlledQROMGen(
            CircuitBuilder* builder,
            bool ctrl,
            std::uint64_t address_size,
            std::uint64_t value_size
    )
        : CircuitGenerator(builder)
        , ctrl{ctrl}
        , address_size{address_size}
        , value_size{value_size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("cs", Type::Qubit, ControlSize, Attribute::Operate);
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, ControlSize + address_size - 1, Attribute::CleanAncilla);
    }
    BigInt GetValue(const BigInt& x) const {
        const auto max_value = BigInt{1} << value_size;
        return (x * BigInt{12323474} + 234956789) % max_value;
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto cs = GetQubits(0);
        auto address = GetQubits(1);
        auto value = GetQubits(2);
        auto aux = GetQubits(3);

        const auto max_address = std::size_t{1} << address_size;

        if (ctrl) {
            for (const auto& q : cs) {
                frontend::gate::X(q);
            }
        }
        auto dict = GetTemporalRegisters(value_size * max_address);
        for (auto i = std::size_t{0}; i < max_address; ++i) {
            frontend::gate::WriteRegisters(GetValue(i), dict.Range(i * value_size, value_size));
        }
        qret::frontend::gate::MultiControlledQROM(cs, address, value, aux, dict);

        return EndCircuitDefinition();
    }
};
TEST(MultiControlledQROM, UsingToffoliStateCtrlTrue) {
    const auto address_size = 5;
    const auto value_size = 6;

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestMultiControlledQROMGen(&builder, true, address_size, value_size);
    auto* circuit = gen.Generate();

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(gen.ControlSize, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ((1 << gen.ControlSize) - 1, state.GetValue(0, gen.ControlSize));
        EXPECT_EQ(address_value, state.GetValue(gen.ControlSize, address_size));
        EXPECT_EQ(
                gen.GetValue(address_value),
                state.GetValue(gen.ControlSize + address_size, value_size)
        );
    }
}
TEST(MultiControlledQROM, UsingToffoliStateCtrlFalse) {
    const auto address_size = 5;
    const auto value_size = 6;

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestMultiControlledQROMGen(&builder, false, address_size, value_size);
    auto* circuit = gen.Generate();

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(gen.ControlSize, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(0, state.GetValue(0, gen.ControlSize));
        EXPECT_EQ(address_value, state.GetValue(gen.ControlSize, address_size));
        EXPECT_EQ(0, state.GetValue(gen.ControlSize + address_size, value_size));
    }
}
//--------------------------------------------------//
// Test QROMImm
//--------------------------------------------------//
TEST(QROMImm, UsingToffoliState) {
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

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto gen = MultiControlledQROMImmGen(&builder, 0, address_size, value_size, dict);
        auto* circuit = gen.Generate();

        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(0, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(dict[address_value], state.GetValue(address_size, value_size));
    }
}
//--------------------------------------------------//
// Test MultiControlledQROMImmGen
//--------------------------------------------------//
TEST(MultiControlledQROMImmGen, UsingToffoliStateCtrlTrue) {
    using qret::frontend::gate::MultiControlledQROMImmGen;
    const auto ctrl_size = std::size_t{2};
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

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto gen = MultiControlledQROMImmGen(&builder, ctrl_size, address_size, value_size, dict);
        auto* circuit = gen.Generate();

        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
            simulator.SetAs1(i);  // ctrl-on
        }
        simulator.SetValue(ctrl_size, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();

        for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
            EXPECT_EQ(1, state.GetValue(i, 1));
        }
        EXPECT_EQ(address_value, state.GetValue(ctrl_size, address_size));
        EXPECT_EQ(dict[address_value], state.GetValue(ctrl_size + address_size, value_size));
    }
}
TEST(MultiControlledQROMImmGen, UsingToffoliStateCtrlFalse) {
    using qret::frontend::gate::MultiControlledQROMImmGen;
    const auto ctrl_size = std::size_t{2};
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

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto gen = MultiControlledQROMImmGen(&builder, ctrl_size, address_size, value_size, dict);
        auto* circuit = gen.Generate();

        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        // for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
        //     simulator.SetAs1(i);  // ctrl-off
        // }
        simulator.SetValue(ctrl_size, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();

        for (auto i = std::size_t{0}; i < ctrl_size; ++i) {
            EXPECT_EQ(0, state.GetValue(i, 1));
        }
        EXPECT_EQ(address_value, state.GetValue(ctrl_size, address_size));
        EXPECT_EQ(0, state.GetValue(ctrl_size + address_size, value_size));
    }
}
//--------------------------------------------------//
// Test UncomputeQROM
//--------------------------------------------------//
struct TestUncomputeQROMGen : public CircuitGenerator {
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "UncomputeQROM";
    explicit TestUncomputeQROMGen(
            CircuitBuilder* builder,
            std::uint64_t address_size,
            std::uint64_t value_size
    )
        : CircuitGenerator(builder)
        , address_size{address_size}
        , value_size{value_size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("aux", Type::Qubit, address_size - 1, Attribute::CleanAncilla);
    }
    BigInt GetValue(const BigInt& x) const {
        const auto max_value = BigInt{1} << value_size;
        return (x * BigInt{12323474} + 234956789) % max_value;
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto aux = GetQubits(2);

        const auto max_address = std::size_t{1} << address_size;
        auto dict = GetTemporalRegisters(value_size * max_address);
        for (auto i = std::size_t{0}; i < max_address; ++i) {
            frontend::gate::WriteRegisters(GetValue(i), dict.Range(i * value_size, value_size));
        }

        qret::frontend::gate::QROM(address, value, aux, dict);
        qret::frontend::gate::UncomputeQROM(address, value, aux, dict);

        return EndCircuitDefinition();
    }
};
TEST(TestUncomputeQROM, UsingToffoliState) {
    const auto address_size = 5;
    const auto value_size = 6;

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestUncomputeQROMGen(&builder, address_size, value_size);
    auto* circuit = gen.Generate();

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 8;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(0, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(0, state.GetValue(address_size, value_size));
    }
}
//--------------------------------------------------//
// Test UncomputeQROMImm
//--------------------------------------------------//
struct TestUncomputeQROMImmGen : public CircuitGenerator {
    const std::vector<BigInt>& dict;
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "UncomputeQROMImm";
    explicit TestUncomputeQROMImmGen(
            CircuitBuilder* builder,
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
    frontend::Circuit* Generate() const override {
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
TEST(TestUncomputeQROMImm, UsingToffoliState) {
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

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestUncomputeQROMImmGen(&builder, address_size, value_size, dict);
    auto* circuit = gen.Generate();

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 8;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(0, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(0, state.GetValue(address_size, value_size));
    }
}
//--------------------------------------------------//
// Test QROAMClean
//--------------------------------------------------//
struct TestQROAMCleanGen : public CircuitGenerator {
    std::uint64_t num_par;
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "TestQROAMClean";
    explicit TestQROAMCleanGen(
            CircuitBuilder* builder,
            std::uint64_t num_par,
            std::uint64_t address_size,
            std::uint64_t value_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , address_size{address_size}
        , value_size{value_size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("duplicate",
                Type::Qubit,
                value_size * ((std::size_t{1} << num_par) - 1),
                Attribute::Operate);
        arg.Add("aux", Type::Qubit, address_size - num_par - 1, Attribute::CleanAncilla);
    }
    BigInt GetValue(const BigInt& x) const {
        const auto max_value = BigInt{1} << value_size;
        return (x * BigInt{12323474} + 234956789) % max_value;
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto duplicate = GetQubits(2);
        auto aux = GetQubits(3);

        const auto max_address = std::size_t{1} << address_size;

        // Initialize register
        auto dict = GetTemporalRegisters(value_size * max_address);
        for (auto i = std::size_t{0}; i < max_address; ++i) {
            frontend::gate::WriteRegisters(GetValue(i), dict.Range(i * value_size, value_size));
        }

        // Run QROAM
        qret::frontend::gate::QROAMClean(num_par, address, value, duplicate, aux, dict);

        return EndCircuitDefinition();
    }
};
TEST(TestQROAMClean, UsingToffoliState) {
    const auto num_par = 2;
    const auto address_size = 6;
    const auto value_size = 9;

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestQROAMCleanGen(&builder, num_par, address_size, value_size);
    auto* circuit = gen.Generate();

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(0, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(gen.GetValue(address_value), state.GetValue(address_size, value_size));
    }
}
//--------------------------------------------------//
// Test UncomputeQROAMClean
//--------------------------------------------------//
struct TestUncomputeQROAMCleanGen : public CircuitGenerator {
    std::uint64_t num_par;
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "TestUncomputeQROAMClean";
    explicit TestUncomputeQROAMCleanGen(
            CircuitBuilder* builder,
            std::uint64_t num_par,
            std::uint64_t address_size,
            std::uint64_t value_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , address_size{address_size}
        , value_size{value_size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("duplicate",
                Type::Qubit,
                value_size * ((std::size_t{1} << num_par) - 1),
                Attribute::Operate);
        arg.Add("aux", Type::Qubit, address_size - num_par - 1, Attribute::CleanAncilla);
    }
    BigInt GetValue(const BigInt& x) const {
        const auto max_value = BigInt{1} << value_size;
        return (x * BigInt{12323474} + 234956789) % max_value;
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto duplicate = GetQubits(2);
        auto aux = GetQubits(3);

        const auto max_address = std::size_t{1} << address_size;

        // Initialize register
        auto dict = GetTemporalRegisters(value_size * max_address);
        for (auto i = std::size_t{0}; i < max_address; ++i) {
            frontend::gate::WriteRegisters(GetValue(i), dict.Range(i * value_size, value_size));
        }

        // Run QROAM
        qret::frontend::gate::QROAMClean(num_par, address, value, duplicate, aux, dict);
        for (const auto& q : duplicate) {
            qret::frontend::gate::Reset(q);
        }
        qret::frontend::gate::UncomputeQROAMClean(
                num_par,
                address,
                value,
                duplicate.Range(0, 1 << num_par),
                aux,
                dict
        );

        return EndCircuitDefinition();
    }
};
TEST(TestUncomputeQROAMClean, UsingToffoliState) {
    const auto num_par = 2;
    const auto address_size = 6;
    const auto value_size = 4;

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestUncomputeQROAMCleanGen(&builder, num_par, address_size, value_size);
    auto* circuit = gen.Generate();

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 32;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(0, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(0, state.GetValue(address_size, value_size));
    }
}
//--------------------------------------------------//
// Test QROAMDirty
//--------------------------------------------------//
struct TestQROAMDirtyGen : public CircuitGenerator {
    std::uint64_t num_par;
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "TestQROAMDirty";
    explicit TestQROAMDirtyGen(
            CircuitBuilder* builder,
            std::uint64_t num_par,
            std::uint64_t address_size,
            std::uint64_t value_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , address_size{address_size}
        , value_size{value_size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("duplicate",
                Type::Qubit,
                value_size * ((std::size_t{1} << num_par) - 1),
                Attribute::CleanAncilla);
        arg.Add("aux", Type::Qubit, address_size - num_par - 1, Attribute::CleanAncilla);
    }
    BigInt GetValue(const BigInt& x) const {
        const auto max_value = BigInt{1} << value_size;
        return (x * BigInt{12323474} + 234956789) % max_value;
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto duplicate = GetQubits(2);
        auto aux = GetQubits(3);

        const auto max_address = std::size_t{1} << address_size;

        // Initialize register
        auto dict = GetTemporalRegisters(value_size * max_address);
        for (auto i = std::size_t{0}; i < max_address; ++i) {
            frontend::gate::WriteRegisters(GetValue(i), dict.Range(i * value_size, value_size));
        }

        // Run QROAM
        qret::frontend::gate::QROAMDirty(num_par, address, value, duplicate, aux, dict);

        return EndCircuitDefinition();
    }
};
TEST(TestQROAMDirty, UsingToffoliState) {
    const auto num_par = 2;
    const auto address_size = 6;
    const auto value_size = 5;

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestQROAMDirtyGen(&builder, num_par, address_size, value_size);
    auto* circuit = gen.Generate();

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 64;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(0, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(gen.GetValue(address_value), state.GetValue(address_size, value_size));
    }
}
//--------------------------------------------------//
// Test UncomputeQROAMDirty
//--------------------------------------------------//
struct TestUncomputeQROAMDirtyGen : public CircuitGenerator {
    std::uint64_t num_par;
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "TestUncomputeQROAMDirty";
    explicit TestUncomputeQROAMDirtyGen(
            CircuitBuilder* builder,
            std::uint64_t num_par,
            std::uint64_t address_size,
            std::uint64_t value_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , address_size{address_size}
        , value_size{value_size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("duplicate",
                Type::Qubit,
                value_size * ((std::size_t{1} << num_par) - 1),
                Attribute::CleanAncilla);
        arg.Add("aux", Type::Qubit, address_size - num_par - 1, Attribute::CleanAncilla);
    }
    BigInt GetValue(const BigInt& x) const {
        const auto max_value = BigInt{1} << value_size;
        return (x * BigInt{12323474} + 234956789) % max_value;
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto duplicate = GetQubits(2);
        auto aux = GetQubits(3);

        const auto max_address = std::size_t{1} << address_size;

        // Initialize register
        auto dict = GetTemporalRegisters(value_size * max_address);
        for (auto i = std::size_t{0}; i < max_address; ++i) {
            frontend::gate::WriteRegisters(GetValue(i), dict.Range(i * value_size, value_size));
        }

        // Run QROAM
        qret::frontend::gate::QROAMDirty(num_par, address, value, duplicate, aux, dict);
        qret::frontend::gate::UncomputeQROAMDirty(
                num_par,
                address,
                value,
                duplicate.Range(0, (1 << num_par) - 1),
                duplicate[1 << num_par],
                aux,
                dict
        );

        return EndCircuitDefinition();
    }
};
TEST(TestUncomputeQROAMDirty, UsingToffoliState) {
    const auto num_par = 2;
    const auto address_size = 6;
    const auto value_size = 5;

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestUncomputeQROAMDirtyGen(&builder, num_par, address_size, value_size);
    auto* circuit = gen.Generate();

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 64;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(0, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(0, state.GetValue(address_size, value_size));
    }
}
//--------------------------------------------------//
// Test QROMParallel
//--------------------------------------------------//
struct TestQROMParallelGen : public CircuitGenerator {
    std::uint64_t num_par;
    std::uint64_t address_size;
    std::uint64_t value_size;

    static inline const char* Name = "TestQROMParallel";
    explicit TestQROMParallelGen(
            CircuitBuilder* builder,
            std::uint64_t num_par,
            std::uint64_t address_size,
            std::uint64_t value_size
    )
        : CircuitGenerator(builder)
        , num_par{num_par}
        , address_size{address_size}
        , value_size{value_size} {}
    std::string GetName() const override {
        return Name;
    }
    // std::string GetCacheKey() const override;
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, address_size, Attribute::Operate);
        arg.Add("value", Type::Qubit, value_size, Attribute::Operate);
        arg.Add("duplicate_address",
                Type::Qubit,
                address_size * ((std::size_t{1} << num_par) - 1),
                Attribute::CleanAncilla);
        arg.Add("duplicate_value",
                Type::Qubit,
                (address_size - 1) * (std::size_t{1} << num_par),
                Attribute::CleanAncilla);
        arg.Add("aux", Type::Qubit, address_size - num_par - 1, Attribute::CleanAncilla);
    }
    BigInt GetValue(const BigInt& x) const {
        const auto max_value = BigInt{1} << value_size;
        return (x * BigInt{12323474} + 234956789) % max_value;
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto address = GetQubits(0);
        auto value = GetQubits(1);
        auto duplicate_address = GetQubits(2);
        auto duplicate_aux = GetQubits(3);

        const auto max_address = std::size_t{1} << address_size;

        // Initialize register
        auto dict = GetTemporalRegisters(value_size * max_address);
        for (auto i = std::size_t{0}; i < max_address; ++i) {
            frontend::gate::WriteRegisters(GetValue(i), dict.Range(i * value_size, value_size));
        }

        // Run QROM
        qret::frontend::gate::QROMParallel(
                num_par,
                address,
                value,
                duplicate_address,
                duplicate_aux,
                dict
        );

        return EndCircuitDefinition();
    }
};
TEST(TestQROMParallel, UsingToffoliState) {
    const auto num_par = 2;
    const auto address_size = 6;
    const auto value_size = 5;

    ir::IRContext context;
    auto* module = ir::Module::Create("qrom", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestQROMParallelGen(&builder, num_par, address_size, value_size);
    auto* circuit = gen.Generate();

    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 2;
        auto simulator = Simulator(config, circuit->GetIR());
        simulator.SetValue(0, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(gen.GetValue(address_value), state.GetValue(address_size, value_size));
    }
}
