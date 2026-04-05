#include "qret/ir/json.h"

#include <gtest/gtest.h>

#include <iostream>

#include "qret/base/type.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/frontend/circuit_generator.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/context.h"
#include "qret/ir/metadata.h"
#include "qret/ir/value.h"
#include "qret/runtime/simulator.h"

#include "test_utils.h"

using namespace qret;
using qret::frontend::CircuitBuilder, qret::frontend::CircuitGenerator;
using runtime::SimulatorConfig, runtime::Simulator;

namespace {
Json MakeFunction(
        const std::string& name,
        const Json& bb_list,
        std::size_t num_qubits,
        std::size_t num_registers,
        std::size_t num_tmp_registers = 0
) {
    auto function = Json::object();
    function["name"] = name;
    function["entry_point"] = bb_list[0]["name"];
    function["bb_list"] = bb_list;
    function["argument"]["num_qubits"] = num_qubits;
    function["argument"]["qubits"]["q"] = num_qubits;
    function["argument"]["num_registers"] = num_registers;
    function["argument"]["registers"]["r"] = num_registers;
    function["num_tmp_registers"] = num_tmp_registers;
    return function;
}
Json MakeModule(const std::string& name, const Json& circuit_list) {
    auto module = Json::object();
    module["name"] = name;
    module["circuit_list"] = circuit_list;
    return module;
}
}  // namespace

TEST(TestIRSerialization, AddCuccaro) {
    ir::IRContext context;
    tests::LoadContextFromJsonFile("quration-core/tests/data/circuit/add_cuccaro_5.json", context);
    auto j = Json();
    j = *context.owned_module.back();
    std::cout << j.dump(2) << std::endl;
}
TEST(TestIRSerialization, AddCraig) {
    ir::IRContext context;
    tests::LoadContextFromJsonFile("quration-core/tests/data/circuit/add_craig_5.json", context);
    auto j = Json();
    j = *context.owned_module.back();
    std::cout << j.dump(2) << std::endl;
}
TEST(TestIRDeserialization, AddCuccaro) {
    auto size = std::size_t{5};
    const auto dst_value = 10;
    const auto src_value = 17;

    ir::IRContext context;
    tests::LoadContextFromJsonFile("quration-core/tests/data/circuit/add_cuccaro_5.json", context);
    const auto& module = *context.owned_module.back();
    std::cout << "Module: " << module.GetName() << std::endl;
    for (const auto& func : module) {
        std::cout << "Function: " << func.GetName() << std::endl;
        for (const auto& bb : func) {
            std::cout << "BB: " << bb.GetName() << std::endl;
            for (const auto& inst : bb) {
                std::cout << "  ";
                inst.Print(std::cout);
                std::cout << std::endl;
            }
        }
    }

    // run deserialized IR
    const auto* func = context.owned_module.back()->GetFunction("AddCuccaro(5)");
    ASSERT_NE(nullptr, func);
    const auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    auto simulator = Simulator(config, func);
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(dst_value + src_value, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}
TEST(TestIRDeserialization, AddCraig) {
    const auto size = std::size_t{5};
    const auto dst_value = 10;
    const auto src_value = 17;

    ir::IRContext context;
    tests::LoadContextFromJsonFile("quration-core/tests/data/circuit/add_craig_5.json", context);
    const auto& module = *context.owned_module.back();
    std::cout << "Module: " << module.GetName() << std::endl;
    for (const auto& func : module) {
        std::cout << "Function: " << func.GetName() << std::endl;
        for (const auto& bb : func) {
            std::cout << "BB: " << bb.GetName() << std::endl;
            for (const auto& inst : bb) {
                std::cout << "  ";
                inst.Print(std::cout);
                std::cout << std::endl;
            }
        }
    }

    // run deserialized IR
    const auto* func = context.owned_module.back()->GetFunction("AddCraig(5)");
    ASSERT_NE(nullptr, func);
    auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
    config.max_superpositions = 2;
    auto simulator = Simulator(config, func);
    simulator.SetValue(0, size, dst_value);
    simulator.SetValue(size, size, src_value);
    simulator.RunAll();

    const auto& state = simulator.GetToffoliState();
    EXPECT_EQ(dst_value + src_value, state.GetValue(0, size));
    EXPECT_EQ(src_value, state.GetValue(size, size));
}

TEST(TestClassicalFunction, UsingToffoliState) {
    const auto address_size = 5;
    const auto value_size = 6;

    ir::IRContext context;
    tests::LoadContextFromJsonFile(
            "quration-core/tests/data/circuit/test_classical_function.json",
            context
    );
    const auto& module = *context.owned_module.back();
    std::cout << "Module: " << module.GetName() << std::endl;
    for (const auto& func : module) {
        std::cout << "Function: " << func.GetName() << std::endl;
        for (const auto& bb : func) {
            std::cout << "BB: " << bb.GetName() << std::endl;
            for (const auto& inst : bb) {
                std::cout << "  ";
                inst.Print(std::cout);
                std::cout << std::endl;
            }
        }
    }

    // run deserialized IR
    const auto* func =
            context.owned_module.back()->GetFunction("TestClassicalFunction__000000000__");
    ASSERT_NE(nullptr, func);
    for (auto address_value = 0; address_value < (1 << address_size); ++address_value) {
        auto config = SimulatorConfig{SimulatorConfig::StateType::Toffoli};
        config.max_superpositions = 8;
        auto simulator = Simulator(config, func);
        simulator.SetValue(0, address_size, address_value);
        simulator.RunAll();

        const auto& state = simulator.GetToffoliState();
        EXPECT_EQ(address_value, state.GetValue(0, address_size));
        EXPECT_EQ(0, state.GetValue(address_size, value_size));
    }
}
struct TestMetadataSerializationGen : public CircuitGenerator {
    static inline const char* Name = "TestMetadataSerialization";
    explicit TestMetadataSerializationGen(CircuitBuilder* builder)
        : CircuitGenerator(builder) {}

    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return Name;
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("q0", Type::Qubit, 1, Attribute::Operate);
        arg.Add("r0", Type::Register, 1, Attribute::Output);
    }
    frontend::Circuit* Generate() const override {
        BeginCircuitDefinition();
        auto q0 = GetQubit(0);
        auto r0 = GetRegister(1);
        frontend::gate::H(q0);
        frontend::gate::X(q0)
                ->AddMDAnnotation("test annotation")
                ->AddMDQubit(ir::Qubit(q0.GetId()))
                ->MarkAsBreakPoint("test breakpoint")
                ->AddLocation("test file", 2);

        frontend::gate::Measure(q0, r0)->AddMDRegister(ir::Register(r0.GetId()));

        return EndCircuitDefinition();
    }
};
TEST(TestMetadata, AllMetadata) {
    // serialization
    auto j = Json();
    ir::IRContext context;
    auto* module = ir::Module::Create("json", context);
    auto builder = CircuitBuilder(module);
    auto gen = TestMetadataSerializationGen(&builder);
    gen.Generate();
    {
        j = *builder.GetModule();
        std::cout << j << std::endl;
        const auto inst_list_json = j["circuit_list"][0]["bb_list"][0]["inst_list"];

        // 1st inst without metadata.
        EXPECT_FALSE(inst_list_json[0].contains("metadata"));

        // 2nd inst with four metadata.
        EXPECT_EQ(inst_list_json[1]["metadata"].size(), 4);
        EXPECT_EQ(inst_list_json[1]["metadata"][0]["kind"], "Annotation");
        EXPECT_EQ(inst_list_json[1]["metadata"][0]["annot"], "test annotation");
        EXPECT_EQ(inst_list_json[1]["metadata"][1]["kind"], "Qubit");
        EXPECT_EQ(inst_list_json[1]["metadata"][1]["q"], gen.GetQubit(0).GetId());
        EXPECT_EQ(inst_list_json[1]["metadata"][2]["kind"], "Breakpoint");
        EXPECT_EQ(inst_list_json[1]["metadata"][2]["name"], "test breakpoint");
        EXPECT_EQ(inst_list_json[1]["metadata"][3]["kind"], "Location");
        EXPECT_EQ(inst_list_json[1]["metadata"][3]["file"], "test file");
        EXPECT_EQ(inst_list_json[1]["metadata"][3]["line"], std::uint32_t{2});

        // 3rd inst with one metadata.
        EXPECT_EQ(inst_list_json[2]["metadata"].size(), 1);
        EXPECT_EQ(inst_list_json[2]["metadata"][0]["kind"], "Register");
        EXPECT_EQ(inst_list_json[2]["metadata"][0]["r"], gen.GetRegister(1).GetId());
    }

    // deserialization
    {
        ir::IRContext context_deserialized;
        ir::LoadJson(j, context_deserialized);
        const auto& deserialized_module = *context_deserialized.owned_module.back();
        const auto* deserialized_func =
                deserialized_module.GetFunction("TestMetadataSerialization");
        ASSERT_NE(nullptr, deserialized_func);

        const auto* bb = deserialized_func->GetEntryBB();
        EXPECT_EQ(bb->Size(), 4);

        // 1st inst
        EXPECT_FALSE(bb->Front().HasMetadata());

        // 2nd inst
        const auto* inst2 = bb->Front().GetNextInstruction();
        EXPECT_TRUE(inst2->HasMetadata());
        EXPECT_EQ(std::distance(inst2->md_begin(), inst2->md_end()), 4);
        EXPECT_EQ(inst2->md_begin()->GetMDKind(), qret::ir::MDKind::Annotation);
        auto md_itr_inst2 = inst2->md_begin();
        EXPECT_EQ(
                static_cast<const qret::ir::MDAnnotation*>(&*md_itr_inst2)->GetAnnotation(),
                "test annotation"
        );
        ++md_itr_inst2;
        EXPECT_EQ(
                static_cast<const qret::ir::MDQubit*>(&*(md_itr_inst2))->GetQubit().id,
                gen.GetQubit(0).GetId()
        );
        ++md_itr_inst2;
        EXPECT_EQ(
                static_cast<const qret::ir::DIBreakPoint*>(&*(md_itr_inst2))->GetName(),
                "test breakpoint"
        );
        ++md_itr_inst2;
        EXPECT_EQ(
                static_cast<const qret::ir::DILocation*>(&*(md_itr_inst2))->GetFile(),
                "test file"
        );
        EXPECT_EQ(static_cast<const qret::ir::DILocation*>(&*(md_itr_inst2))->GetLine(), 2);

        // 3rd inst
        const auto* inst3 = inst2->GetNextInstruction();
        EXPECT_TRUE(inst3->HasMetadata());
        EXPECT_EQ(std::distance(inst3->md_begin(), inst3->md_end()), 1);
        auto md_itr_inst3 = inst3->md_begin();
        EXPECT_EQ(
                static_cast<const qret::ir::MDRegister*>(&*(md_itr_inst3))->GetRegister().id,
                gen.GetRegister(1).GetId()
        );
    }
}

TEST(TestIRDeserializationValidation, DuplicateFunctionName) {
    const auto bb_list = Json::array(
            {Json{{"name", "entry"},
                  {"inst_list", Json::array({Json{{"opcode", "Return"}}})},
                  {"predecessors", Json::array()},
                  {"successors", Json::array()}}}
    );
    const auto function = MakeFunction("dup", bb_list, 1, 1);
    const auto module = MakeModule("mod", Json::array({function, function}));

    ir::IRContext context;
    EXPECT_THROW(ir::LoadJson(module, context), std::runtime_error);
}

TEST(TestIRDeserializationValidation, CallArgumentSize) {
    const auto callee_bb = Json::array(
            {Json{{"name", "entry"},
                  {"inst_list", Json::array({Json{{"opcode", "Return"}}})},
                  {"predecessors", Json::array()},
                  {"successors", Json::array()}}}
    );
    const auto caller_bb = Json::array(
            {Json{{"name", "entry"},
                  {"inst_list",
                   Json::array(
                           {Json{{"opcode", "Call"},
                                 {"callee", "callee"},
                                 {"operate", Json::array({0})},
                                 {"input", Json::array({0})},
                                 {"output", Json::array()}},
                            Json{{"opcode", "Return"}}}
                   )},
                  {"predecessors", Json::array()},
                  {"successors", Json::array()}}}
    );
    const auto callee = MakeFunction("callee", callee_bb, 1, 2);
    const auto caller = MakeFunction("caller", caller_bb, 1, 2);
    const auto module = MakeModule("mod", Json::array({callee, caller}));

    ir::IRContext context;
    EXPECT_THROW(ir::LoadJson(module, context), std::runtime_error);
}

TEST(TestIRDeserializationValidation, ReturnReachability) {
    const auto bb_list = Json::array(
            {Json{{"name", "entry"},
                  {"inst_list",
                   Json::array(
                           {Json{{"opcode", "Branch"}, {"num_successors", 1}, {"if_true", "loop"}}}
                   )},
                  {"predecessors", Json::array()},
                  {"successors", Json::array({"loop"})}},
             Json{{"name", "loop"},
                  {"inst_list",
                   Json::array(
                           {Json{{"opcode", "Branch"}, {"num_successors", 1}, {"if_true", "loop"}}}
                   )},
                  {"predecessors", Json::array({"entry", "loop"})},
                  {"successors", Json::array({"loop"})}},
             Json{{"name", "exit"},
                  {"inst_list", Json::array({Json{{"opcode", "Return"}}})},
                  {"predecessors", Json::array()},
                  {"successors", Json::array()}}}
    );
    const auto function = MakeFunction("f", bb_list, 1, 1);
    const auto module = MakeModule("mod", Json::array({function}));

    ir::IRContext context;
    EXPECT_THROW(ir::LoadJson(module, context), std::runtime_error);
}

TEST(TestIRDeserializationValidation, SwitchRegistersKey) {
    const auto bb_list = Json::array(
            {Json{{"name", "entry"},
                  {"inst_list",
                   Json::array(
                           {Json{{"opcode", "Switch"},
                                 {"registers", Json::array({0})},
                                 {"default", "exit"},
                                 {"case", Json{{"1", "exit"}}}}}
                   )},
                  {"predecessors", Json::array()},
                  {"successors", Json::array({"exit"})}},
             Json{{"name", "exit"},
                  {"inst_list", Json::array({Json{{"opcode", "Return"}}})},
                  {"predecessors", Json::array({"entry"})},
                  {"successors", Json::array()}}}
    );
    const auto function = MakeFunction("f", bb_list, 1, 1);
    const auto module = MakeModule("mod", Json::array({function}));

    ir::IRContext context;
    EXPECT_NO_THROW(ir::LoadJson(module, context));

    auto serialized = Json(*context.owned_module.back());
    const auto& switch_json = serialized["circuit_list"][0]["bb_list"][0]["inst_list"][0];
    EXPECT_TRUE(switch_json.contains("registers"));
    EXPECT_FALSE(switch_json.contains("value"));
}

TEST(TestIRDeserializationValidation, SwitchLegacyValueKeyRejected) {
    const auto bb_list = Json::array(
            {Json{{"name", "entry"},
                  {"inst_list",
                   Json::array(
                           {Json{{"opcode", "Switch"},
                                 {"value", Json::array({0})},
                                 {"default", "exit"},
                                 {"case", Json{{"1", "exit"}}}}}
                   )},
                  {"predecessors", Json::array()},
                  {"successors", Json::array({"exit"})}},
             Json{{"name", "exit"},
                  {"inst_list", Json::array({Json{{"opcode", "Return"}}})},
                  {"predecessors", Json::array({"entry"})},
                  {"successors", Json::array()}}}
    );
    const auto function = MakeFunction("f", bb_list, 1, 1);
    const auto module = MakeModule("mod", Json::array({function}));

    ir::IRContext context;
    EXPECT_THROW(ir::LoadJson(module, context), std::runtime_error);
}
