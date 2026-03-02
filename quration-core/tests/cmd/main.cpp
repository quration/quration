#include "qret/cmd/main.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "qret/cmd/common.h"
#include "qret/parser/openqasm2.h"

static constexpr auto BinName = "qret";

namespace {
std::filesystem::path MakeTempPath(const std::string& suffix) {
    static std::uint64_t counter = 0;
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path()
            / ("qret_cmd_main_" + std::to_string(tick) + "_" + std::to_string(counter++) + suffix);
}

void CompileFixtureToScLsFixedV0(const std::filesystem::path& output_path) {
    const auto output = output_path.string();
    const auto circuit_name = std::string("AddCuccaro(3)");
    const auto input_file = std::string("quration-core/tests/data/circuit/add_cuccaro_3.json");
    const auto topology_file = std::string("quration-core/tests/data/topology/plane.yaml");

    // Build a small SC_LS_FIXED_V0 pipeline state used by asm/profile integration tests.
    auto argv = std::vector<const char*>{
            BinName,
            "compile",
            "--input",
            input_file.c_str(),
            "--function",
            circuit_name.c_str(),
            "--output",
            output.c_str(),
            "--sc_ls_fixed_v0_topology",
            topology_file.c_str(),
    };
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
    EXPECT_TRUE(std::filesystem::exists(output_path));
}

std::string ReadFile(const std::filesystem::path& path) {
    auto ifs = std::ifstream(path);
    EXPECT_TRUE(ifs.good());

    auto ss = std::stringstream();
    ss << ifs.rdbuf();
    return ss.str();
}
}  // namespace

TEST(QretMain, NoArgument) {
    auto argv = std::vector<const char*>{BinName};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMain, Help) {
    auto argv = std::vector<const char*>{BinName, "help"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMain, BH) {
    auto argv = std::vector<const char*>{BinName, "-h"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMain, BBHelp) {
    auto argv = std::vector<const char*>{BinName, "--help"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMain, Version) {
    auto argv = std::vector<const char*>{BinName, "version"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMain, BV) {
    auto argv = std::vector<const char*>{BinName, "-v"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMain, BBVersion) {
    auto argv = std::vector<const char*>{BinName, "--version"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMain, UnknownCommand) {
    auto argv = std::vector<const char*>{BinName, "--hep"};
    EXPECT_EQ(1, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMainCompile, BBHelp) {
    auto argv = std::vector<const char*>{BinName, "compile", "--help"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMainAsm, BBHelp) {
    auto argv = std::vector<const char*>{BinName, "asm", "--help"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMainParse, BBHelp) {
    auto argv = std::vector<const char*>{BinName, "parse", "--help"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMainProfile, BBHelp) {
    auto argv = std::vector<const char*>{BinName, "profile", "--help"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMainSimulate, BBHelp) {
    auto argv = std::vector<const char*>{BinName, "simulate", "--help"};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
}
TEST(QretMainAsm, FromScLsFixedV0PipelineState) {
    const auto pipeline_state_file = MakeTempPath(".pipeline_state.json");
    const auto asm_file = MakeTempPath(".out.asm");
    CompileFixtureToScLsFixedV0(pipeline_state_file);

    const auto input = pipeline_state_file.string();
    const auto output = asm_file.string();
    auto argv = std::vector<const char*>{BinName, "asm", input.c_str(), output.c_str()};
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
    EXPECT_TRUE(std::filesystem::exists(asm_file));
    EXPECT_FALSE(ReadFile(asm_file).empty());
}
TEST(QretMainProfile, JsonAndMarkdownFromScLsFixedV0PipelineState) {
    const auto pipeline_state_file = MakeTempPath(".pipeline_state.json");
    const auto json_file = MakeTempPath(".compile_info.json");
    const auto markdown_file = MakeTempPath(".compile_info.md");
    CompileFixtureToScLsFixedV0(pipeline_state_file);

    const auto input = pipeline_state_file.string();
    const auto json_output = json_file.string();
    auto argv_json =
            std::vector<const char*>{BinName, "profile", input.c_str(), json_output.c_str()};
    EXPECT_EQ(0, qret::cmd::QretMain(argv_json.size(), argv_json.data()));
    EXPECT_TRUE(std::filesystem::exists(json_file));
    EXPECT_NE(std::string::npos, ReadFile(json_file).find("\"runtime\""));

    const auto markdown_output = markdown_file.string();
    auto argv_markdown = std::vector<const char*>{
            BinName,
            "profile",
            "--format",
            "markdown",
            input.c_str(),
            markdown_output.c_str(),
    };
    EXPECT_EQ(0, qret::cmd::QretMain(argv_markdown.size(), argv_markdown.data()));
    EXPECT_TRUE(std::filesystem::exists(markdown_file));
    EXPECT_NE(std::string::npos, ReadFile(markdown_file).find("Compile information"));
}
TEST(QretMainCompile, OpenQASM2Source) {
    if (!qret::openqasm2::CanParseOpenQASM2()) {
        GTEST_SKIP() << "Skip OpenQASM2 test";
    }
    const auto output = "qret_compile_openqasm2_from_cmd.json";
    auto argv = std::vector<const char*>{
            BinName,
            "compile",
            "--input",
            "quration-core/tests/data/OpenQASM2/x.qasm",
            "--source",
            "OpenQASM2",
            "--target",
            "SC_LS_FIXED_V0",
            "--output",
            output,
            "--sc_ls_fixed_v0_topology",
            "quration-core/tests/data/topology/plane.yaml",
            "--sc_ls_fixed_v0_pass",
            "sc_ls_fixed_v0::mapping,sc_ls_fixed_v0::routing",
    };
    EXPECT_EQ(0, qret::cmd::QretMain(argv.size(), argv.data()));
    EXPECT_TRUE(std::filesystem::exists(output));
}
