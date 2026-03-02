/**
 * @file qret/cmd/simulate.cpp
 * @brief Define 'simulate' sumcommand in qret.
 */

#include "qret/cmd/simulate.h"

#include <boost/program_options.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <cctype>
#include <complex>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "qret/base/log.h"
#include "qret/base/type.h"
#include "qret/cmd/common.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/json.h"
#include "qret/runtime/full_quantum_state.h"
#include "qret/runtime/simulator.h"

namespace qret::cmd {
namespace {

std::string NormalizeState(std::string_view raw_state) {
    auto cleaned = std::string{};
    cleaned.reserve(raw_state.size());
    for (auto c : raw_state) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0 || c == '_') {
            continue;
        }
        if (c == ',') {
            continue;
        }
        cleaned.push_back(c);
    }
    if (cleaned.size() >= 2 && cleaned.starts_with("0b")) {
        cleaned = cleaned.substr(2);
    }
    if (cleaned.size() >= 2 && cleaned.starts_with("0B")) {
        cleaned = cleaned.substr(2);
    }
    return cleaned;
}

std::string FormatModelLabel(runtime::SimulatorConfig::StateType state) {
    switch (state) {
        case runtime::SimulatorConfig::StateType::FullQuantum:
            return "FullQuantum";
        case runtime::SimulatorConfig::StateType::Toffoli:
            return "Toffoli";
        case runtime::SimulatorConfig::StateType::Clifford:
            return "Clifford";
        case runtime::SimulatorConfig::StateType::MPS:
            return "MPS";
        default:
            return "Unknown";
    }
}

runtime::SimulatorConfig::StateType ParseState(std::string_view state) {
    if (state.empty()) {
        throw std::invalid_argument("state model is empty");
    }

    auto normalized = std::string{};
    normalized.reserve(state.size());
    for (const auto ch : state) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (normalized == "fullquantum" || normalized == "full_quantum" || normalized == "full"
        || normalized == "fq") {
        return runtime::SimulatorConfig::StateType::FullQuantum;
    }
    if (normalized == "toffoli" || normalized == "tof") {
        return runtime::SimulatorConfig::StateType::Toffoli;
    }

    throw std::invalid_argument(
            fmt::format("unknown simulation model: '{}'. expected FullQuantum|Toffoli", state)
    );
}

std::vector<bool> ParseInitState(std::string_view raw_state, std::uint64_t num_qubits) {
    const auto state = NormalizeState(raw_state);
    if (state.empty()) {
        return std::vector<bool>(static_cast<std::size_t>(num_qubits), false);
    }
    if (state.size() > num_qubits) {
        throw std::invalid_argument(
                fmt::format(
                        "initial state length ({}) exceeds qubit count ({})",
                        state.size(),
                        num_qubits
                )
        );
    }

    auto bits = std::vector<bool>(static_cast<std::size_t>(num_qubits), false);
    for (std::size_t i = 0; i < state.size(); ++i) {
        if (state[i] == '0') {
            bits[i] = false;
        } else if (state[i] == '1') {
            bits[i] = true;
        } else {
            throw std::invalid_argument(
                    fmt::format(
                            "invalid initial-state character '{}' at position {}. Use only '0' or "
                            "'1'.",
                            state[i],
                            i + 1
                    )
            );
        }
    }
    return bits;
}

std::string ToBitStringFromBoolArray(const std::vector<bool>& bits) {
    auto ret = std::string(bits.size(), '0');
    for (std::size_t i = 0; i < bits.size(); ++i) {
        ret[i] = bits[i] ? '1' : '0';
    }
    return ret;
}

std::string ToBitStringFromIndex(std::uint64_t x, std::uint64_t width) {
    auto ret = std::string(static_cast<std::size_t>(width), '0');
    for (std::uint64_t i = 0; i < width; ++i) {
        if ((x >> i) & 1ULL) {
            ret[static_cast<std::size_t>(i)] = '1';
        }
    }
    return ret;
}

std::vector<bool> ToBoolArrayFromIndex(std::uint64_t x, std::size_t width) {
    auto bits = std::vector<bool>(width, false);
    for (std::size_t i = 0; i < width; ++i) {
        bits[i] = (x >> i) & 1ULL;
    }
    return bits;
}

std::vector<bool> BoolArrayFromToffoliBasis(const runtime::ToffoliState::State& state_bits) {
    auto bits = std::vector<bool>{};
    bits.reserve(state_bits.size());
    for (const auto bit : state_bits) {
        bits.push_back(bit != 0);
    }
    return bits;
}

std::pair<std::vector<bool>, double>
FindMostLikelyBasisState(const runtime::FullQuantumState& state, std::size_t num_qubits) {
    auto best_state = std::vector<bool>(num_qubits, false);
    const auto* state_vec = state.GetStateVector();
    if (state_vec == nullptr || num_qubits == 0) {
        return {best_state, 0.0};
    }

    auto max_prob = std::numeric_limits<double>::lowest();
    const auto dimension = std::size_t{1} << num_qubits;
    for (std::size_t idx = 0; idx < dimension; ++idx) {
        const auto prob = std::norm(state_vec[idx]);
        if (prob > max_prob) {
            max_prob = prob;
            best_state = ToBoolArrayFromIndex(idx, num_qubits);
        }
    }
    return {best_state, max_prob};
}

std::pair<std::vector<bool>, double> FindMostLikelyBasisState(const runtime::ToffoliState& state) {
    const auto& raw_state = state.GetRawVector();
    if (raw_state.empty()) {
        return {{}, 0.0};
    }

    auto best_bits = BoolArrayFromToffoliBasis(raw_state.front().second);
    auto max_prob = std::norm(raw_state.front().first);
    for (const auto& [coef, bits] : raw_state) {
        const auto prob = std::norm(coef);
        if (prob > max_prob) {
            max_prob = prob;
            best_bits = BoolArrayFromToffoliBasis(bits);
        }
    }
    return {best_bits, max_prob};
}

std::string FormatComplex(std::complex<double> value) {
    return fmt::format("({:+.6f}{:+.6f}i)", value.real(), value.imag());
}

namespace {
constexpr std::size_t kIndexWidth = 4;
constexpr std::size_t kAmplitudeWidth = 22;
constexpr std::size_t kProbabilityWidth = 12;

std::string FormatCell(std::string_view value, std::size_t width, bool right_aligned = false) {
    if (right_aligned) {
        return fmt::format("{:>{}}", value, width);
    }
    return fmt::format("{:<{}}", value, width);
}

void PrintMdRow(
        std::ostream& out,
        std::string_view idx,
        std::string_view basis,
        std::string_view val,
        std::string_view prob,
        std::size_t idx_width,
        std::size_t basis_width,
        std::size_t val_width,
        std::size_t prob_width
) {
    out << "  | " << FormatCell(idx, idx_width, true) << " | "
        << FormatCell(basis, basis_width, false) << " | " << FormatCell(val, val_width, false)
        << " | " << FormatCell(prob, prob_width, false) << " |\n";
}

void PrintMdHeader(
        std::ostream& out,
        std::string_view idx_label,
        std::string_view basis_label,
        std::string_view val_label,
        std::string_view prob_label,
        std::size_t idx_width,
        std::size_t basis_width,
        std::size_t val_width,
        std::size_t prob_width
) {
    out << "  | " << FormatCell(idx_label, idx_width, false) << " | "
        << FormatCell(basis_label, basis_width, false) << " | "
        << FormatCell(val_label, val_width, false) << " | "
        << FormatCell(prob_label, prob_width, false) << " |\n";
    out << "  |" << std::string(idx_width + 2, '-') << "|" << std::string(basis_width + 2, '-')
        << "|" << std::string(val_width + 2, '-') << "|" << std::string(prob_width + 2, '-')
        << "|\n";
}

}  // namespace

void PrintSection(std::ostream& out, std::string_view section) {
    out << "[" << section << "]\n";
}

void PrintRunHeader(std::ostream& out, std::uint64_t run_index, std::uint64_t total_runs) {
    out << "\n[Run " << run_index << "/" << total_runs << "]\n";
}

void PrintConfigLine(std::ostream& out, std::string_view key, std::string_view value) {
    out << "  " << fmt::format("{:<20}", key) << value << "\n";
}

std::size_t EstimateNameFieldWidth(const std::vector<std::pair<std::string, std::size_t>>& fields) {
    auto width = std::size_t{0};
    for (const auto& [name, _] : fields) {
        width = std::max(width, name.size());
    }
    if (fields.empty()) {
        width = std::max(width, std::string_view("name").size());
    }
    return width;
}

std::uint64_t
BoolArraySliceToInt(const std::vector<bool>& bits, std::size_t start, std::size_t width) {
    auto value = std::uint64_t{0};
    for (std::size_t i = 0; i < width && start + i < bits.size(); ++i) {
        if (bits[start + i]) {
            value |= (std::uint64_t{1} << i);
        }
    }
    return value;
}

std::string ToQubitRange(std::size_t start, std::size_t width) {
    if (width == 0) {
        return "q[]";
    }
    if (width == 1) {
        return fmt::format("q{}", start);
    }
    return fmt::format("q[{}..{}]", start, start + width - 1);
}

std::string ToRegisterRange(std::size_t start, std::size_t width) {
    if (width == 0) {
        return "r[]";
    }
    if (width == 1) {
        return fmt::format("r{}", start);
    }
    return fmt::format("r[{}..{}]", start, start + width - 1);
}

std::string SliceToBitString(const std::vector<bool>& bits, std::size_t start, std::size_t width) {
    auto bitslice = std::string{};
    bitslice.reserve(width);
    for (std::size_t i = 0; i < width && start + i < bits.size(); ++i) {
        bitslice.push_back(bits[start + i] ? '1' : '0');
    }
    return bitslice.empty() ? "(out of range)" : bitslice;
}

void PrintArgumentValues(
        std::ostream& out,
        const ir::Function* func,
        const std::vector<bool>& bits,
        std::string_view section_name
) {
    const auto& argument = func->GetArgument();
    if (argument.qubits.empty()) {
        return;
    }

    PrintSection(out, section_name);
    const auto name_width = std::max<std::size_t>(4, EstimateNameFieldWidth(argument.qubits));
    const auto bit_range_width = std::string{"bit_range"}.size();
    auto bits_width = std::string{"bits"}.size();
    auto value_width = std::string{"value"}.size();
    for (const auto& [_, size] : argument.qubits) {
        bits_width = std::max(bits_width, size);
        const auto max_value = (size >= 64) ? std::numeric_limits<std::uint64_t>::max()
                                            : ((std::uint64_t{1} << size) - 1);
        value_width = std::max(value_width, std::to_string(max_value).size());
    }

    out << "  | " << FormatCell("name", name_width, false) << " | " << FormatCell("size", 4, false)
        << " | " << FormatCell("bit_range", bit_range_width, false) << " | "
        << FormatCell("bits", bits_width, false) << " | " << FormatCell("value", value_width, false)
        << " |\n";
    out << "  |" << std::string(name_width + 2, '-') << "|" << std::string(4 + 2, '-') << "|"
        << std::string(bit_range_width + 2, '-') << "|" << std::string(bits_width + 2, '-') << "|"
        << std::string(value_width + 2, '-') << "|\n";

    auto cursor = std::size_t{0};
    for (const auto& [name, size] : argument.qubits) {
        const auto bits_str = SliceToBitString(bits, cursor, size);
        const auto range = ToQubitRange(cursor, size);
        const auto value = BoolArraySliceToInt(bits, cursor, size);
        out << "  | " << FormatCell(name, name_width, false) << " | "
            << FormatCell(fmt::format("{}", size), 4, true) << " | "
            << FormatCell(range, bit_range_width, false) << " | "
            << FormatCell(bits_str, bits_width, false) << " | "
            << FormatCell(fmt::format("{}", value), value_width, true) << " |\n";

        cursor += size;
        if (cursor >= bits.size()) {
            cursor = bits.size();
            break;
        }
    }
    out << "  note: bit order is q0 q1 ... in printed strings.\n";
}

void PrintRegisterValues(
        std::ostream& out,
        const ir::Function* func,
        const runtime::QuantumState& state,
        std::string_view section_name
) {
    const auto& argument = func->GetArgument();
    if (argument.registers.empty()) {
        return;
    }

    PrintSection(out, section_name);
    const auto name_width = std::max<std::size_t>(4, EstimateNameFieldWidth(argument.registers));
    const auto bit_range_width = std::string{"bit_range"}.size();
    auto bits_width = std::string{"bits"}.size();
    auto value_width = std::string{"value"}.size();
    for (const auto& [_, size] : argument.registers) {
        bits_width = std::max(bits_width, size);
        const auto max_value = (size >= 64) ? std::numeric_limits<std::uint64_t>::max()
                                            : ((std::uint64_t{1} << size) - 1);
        value_width = std::max(value_width, std::to_string(max_value).size());
    }

    out << "  | " << FormatCell("name", name_width, false) << " | " << FormatCell("size", 4, false)
        << " | " << FormatCell("bit_range", bit_range_width, false) << " | "
        << FormatCell("bits", bits_width, false) << " | " << FormatCell("value", value_width, false)
        << " |\n";
    out << "  |" << std::string(name_width + 2, '-') << "|" << std::string(4 + 2, '-') << "|"
        << std::string(bit_range_width + 2, '-') << "|" << std::string(bits_width + 2, '-') << "|"
        << std::string(value_width + 2, '-') << "|\n";

    auto cursor = std::size_t{0};
    for (const auto& [name, size] : argument.registers) {
        const auto bits = state.ReadRegisters(cursor, size);
        const auto bits_str = SliceToBitString(bits, 0, size);
        const auto range = ToRegisterRange(cursor, size);
        const auto value = BoolArraySliceToInt(bits, 0, size);
        out << "  | " << FormatCell(name, name_width, false) << " | "
            << FormatCell(fmt::format("{}", size), 4, true) << " | "
            << FormatCell(range, bit_range_width, false) << " | "
            << FormatCell(bits_str, bits_width, false) << " | "
            << FormatCell(fmt::format("{}", value), value_width, true) << " |\n";

        cursor += size;
        if (cursor >= argument.num_registers) {
            cursor = argument.num_registers;
            break;
        }
    }
    out << "  note: bit order is r0 r1 ... in printed strings.\n";
}

void PrintFinalState(
        std::ostream& out,
        const ir::Function* func,
        const runtime::QuantumState& state,
        const std::vector<bool>& final_bits,
        double probability
) {
    PrintSection(out, "Final State");
    PrintConfigLine(out, "basis", ToBitStringFromBoolArray(final_bits));
    PrintConfigLine(out, "probability", fmt::format("{:.6f}", probability));
    PrintArgumentValues(out, func, final_bits, "Argument Final State");
    PrintRegisterValues(out, func, state, "Register Final State");
}

void PrintSamples(
        std::ostream& out,
        const ir::Function* func,
        const std::vector<std::uint64_t>& samples
) {
    PrintSection(out, "Samples");
    if (samples.empty()) {
        out << "  (not requested)\n";
        return;
    }

    out << "  count: " << samples.size() << "\n";
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto bool_array = IntAsBoolArray(samples[i], func->GetNumQubits());
        out << "  " << fmt::format("{:>4}", i) << ": " << ToBitStringFromBoolArray(bool_array)
            << " (" << samples[i] << ")\n";
    }
}

void PrintSamplingSummary(
        std::ostream& out,
        const std::vector<std::uint64_t>& samples,
        std::uint64_t num_qubits
) {
    out << "  count: " << samples.size() << "\n";
    if (samples.empty()) {
        out << "  (not requested)\n";
        return;
    }

    auto histogram = std::map<std::string, std::uint64_t>{};
    for (const auto sample : samples) {
        const auto bits = IntAsBoolArray(sample, static_cast<std::size_t>(num_qubits));
        ++histogram[ToBitStringFromBoolArray(bits)];
    }

    auto ranked = std::vector<std::pair<std::string, std::uint64_t>>{};
    ranked.reserve(histogram.size());
    for (const auto& [state, count] : histogram) {
        ranked.emplace_back(state, count);
    }
    std::sort(ranked.begin(), ranked.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second != rhs.second) {
            return lhs.second > rhs.second;
        }
        return lhs.first < rhs.first;
    });

    const auto total = static_cast<double>(samples.size());
    for (const auto& [state, count] : ranked) {
        const auto ratio = 100.0 * static_cast<double>(count) / total;
        out << "  " << state << ": " << count << " (" << fmt::format("{:.2f}", ratio) << "%)\n";
    }
}

void PrintFullQuantumState(
        std::ostream& out,
        const ir::Function* func,
        const runtime::FullQuantumState& state,
        const runtime::QuantumState& raw_state,
        std::uint64_t num_samples,
        bool print_raw
) {
    PrintSection(out, "FullQuantum State");
    PrintConfigLine(out, "num_superpositions", std::to_string(state.GetNumSuperpositions()));
    PrintConfigLine(out, "entropy", fmt::format("{:.12g}", state.GetEntropy()));

    PrintSamples(
            out,
            func,
            num_samples == 0 ? std::vector<std::uint64_t>{} : state.Sampling(num_samples)
    );
    const auto [final_bits, max_prob] = FindMostLikelyBasisState(state, func->GetNumQubits());
    PrintFinalState(out, func, raw_state, final_bits, max_prob);

    if (!print_raw) {
        return;
    }

    PrintSection(out, "State Vector");
    const auto dimension = std::size_t{1} << static_cast<std::size_t>(func->GetNumQubits());
    const auto* state_vec = state.GetStateVector();
    const auto basis_width =
            std::max<std::size_t>(func->GetNumQubits(), std::string{"basis(q0..qN-1)"}.size());
    const auto amp_width = kAmplitudeWidth;
    const auto prob_width = kProbabilityWidth;

    out << "  Note: basis string is ordered as q0, q1, ... (LSB first in index).\n";
    PrintMdHeader(
            out,
            "idx",
            "basis(q0..qN-1)",
            "amplitude",
            "probability",
            kIndexWidth,
            basis_width,
            amp_width,
            prob_width
    );
    for (std::size_t i = 0; i < dimension; ++i) {
        const auto amp = state_vec[i];
        const auto prob = std::norm(amp);
        PrintMdRow(
                out,
                fmt::format("{}", i),
                ToBitStringFromIndex(i, func->GetNumQubits()),
                FormatComplex(amp),
                fmt::format("{:.6f}", prob),
                kIndexWidth,
                basis_width,
                amp_width,
                prob_width
        );
    }
}

void PrintToffoliState(
        std::ostream& out,
        const ir::Function* func,
        const runtime::ToffoliState& state,
        const runtime::QuantumState& run_state
) {
    PrintSection(out, "Toffoli State");
    PrintConfigLine(out, "num_superpositions", std::to_string(state.GetNumSuperpositions()));

    const auto [final_bits, max_prob] = FindMostLikelyBasisState(state);
    PrintFinalState(out, func, run_state, final_bits, max_prob);

    PrintSection(out, "Raw State");
    out << "  global_phase: " << FormatComplex(state.GetPhase()) << "\n";
    const auto basis_width =
            std::max<std::size_t>(func->GetNumQubits(), std::string{"basis(q0..qN-1)"}.size());
    const auto amp_width = kAmplitudeWidth;
    const auto prob_width = kProbabilityWidth;

    PrintMdHeader(
            out,
            "idx",
            "basis(q0..qN-1)",
            "coef",
            "prob",
            kIndexWidth,
            basis_width,
            amp_width,
            prob_width
    );
    const auto& raw_toffoli_state = state.GetRawVector();
    for (auto i = std::size_t{0}; i < raw_toffoli_state.size(); ++i) {
        const auto& [coef, bits] = raw_toffoli_state[i];
        PrintMdRow(
                out,
                fmt::format("{}", i + 1),
                ToBitStringFromBoolArray(std::vector<bool>{bits.begin(), bits.end()}),
                FormatComplex(coef),
                fmt::format("{:.6f}", std::norm(coef)),
                kIndexWidth,
                basis_width,
                amp_width,
                prob_width
        );
    }
}

void PrintConfigSummary(
        std::ostream& out,
        const ir::Function* func,
        const runtime::SimulatorConfig& config,
        const std::vector<bool>& init_state,
        const runtime::QuantumState& state,
        bool print_raw,
        std::uint64_t num_samples
) {
    PrintSection(out, "Run Configuration");
    PrintConfigLine(out, "function", func->GetName());
    PrintConfigLine(out, "qubit_count", std::to_string(func->GetNumQubits()));
    PrintConfigLine(out, "model", FormatModelLabel(config.state_type));
    PrintConfigLine(out, "initial_state", ToBitStringFromBoolArray(init_state));
    PrintConfigLine(out, "seed", std::to_string(config.seed));
    if (config.state_type == runtime::SimulatorConfig::StateType::FullQuantum) {
        PrintConfigLine(out, "num_runs", std::to_string(num_samples));
        PrintConfigLine(out, "print_raw", print_raw ? "true" : "false");
        PrintConfigLine(out, "use_qulacs", config.use_qulacs ? "true" : "false");
    } else if (config.state_type == runtime::SimulatorConfig::StateType::Toffoli) {
        PrintConfigLine(out, "num_runs", std::to_string(num_samples));
        PrintConfigLine(out, "print_raw", print_raw ? "true" : "false");
        PrintConfigLine(out, "max_superpositions", std::to_string(config.max_superpositions));
    }

    PrintArgumentValues(out, func, init_state, "Argument Initialization");
    PrintRegisterValues(out, func, state, "Register Initialization");
}

}  // namespace

ReturnStatus Simulate(
        const std::string& input,
        const std::string& function_name,
        const std::string& state,
        const std::string& init_state,
        std::uint64_t seed,
        std::uint64_t num_samples,
        bool sample_summary,
        bool print_raw,
        bool use_qulacs,
        std::uint64_t max_superpositions
) {
    // Load the input json, which is a serialized IR module.
    LOG_INFO("Load IR.");
    auto ifs = std::ifstream(input);
    auto j = qret::Json::parse(ifs);

    // Deserialize IR module from json
    LOG_INFO("Find function.");
    qret::ir::IRContext context;
    qret::ir::LoadJson(j, context);
    const auto* module = context.owned_module.back().get();
    const auto* func = module->GetFunction(function_name);
    if (func == nullptr) {
        std::cerr << "function of name '" << function_name << "' not found" << std::endl;
        std::cerr << "available functions:" << std::endl;
        auto names = std::vector<std::string>{};
        names.reserve(static_cast<std::size_t>(std::distance(module->begin(), module->end())));
        for (const auto& function : *module) {
            names.push_back(std::string(function.GetName()));
        }
        std::sort(names.begin(), names.end());
        for (const auto& name : names) {
            std::cerr << "  - " << name << std::endl;
        }
        return ReturnStatus::Failure;
    }

    runtime::SimulatorConfig::StateType parsed_state;
    try {
        parsed_state = ParseState(state);
    } catch (const std::invalid_argument& ex) {
        std::cerr << ex.what() << std::endl;
        return ReturnStatus::Failure;
    }

    std::vector<bool> init_bits;
    try {
        init_bits = ParseInitState(init_state, func->GetNumQubits());
    } catch (const std::invalid_argument& ex) {
        std::cerr << "invalid --init_state: " << ex.what() << std::endl;
        return ReturnStatus::Failure;
    }

    const auto run_count = num_samples == 0 ? 1 : num_samples;

    if (sample_summary && parsed_state != runtime::SimulatorConfig::StateType::FullQuantum) {
        std::cerr << "warning: --sample_summary is supported only for FullQuantum. "
                  << "printing full Toffoli output." << std::endl;
    }

    LOG_INFO("Start simulation ({} runs).", run_count);
    auto all_samples = std::vector<std::uint64_t>{};
    all_samples.reserve(static_cast<std::size_t>(run_count));
    for (std::uint64_t run_idx = 0; run_idx < run_count; ++run_idx) {
        // Construct simulator
        const auto config = [&]() {
            auto config = qret::runtime::SimulatorConfig();
            config.state_type = parsed_state;
            config.seed = seed + run_idx;
            config.use_qulacs = use_qulacs;
            config.max_superpositions = std::max<std::uint64_t>(1, max_superpositions);
            return config;
        }();
        auto simulator = qret::runtime::Simulator(config, func);

        // Initialize simulator
        LOG_INFO("Initialize simulator (run {}).", run_idx + 1);
        for (std::uint64_t i = 0; i < init_bits.size(); ++i) {
            if (init_bits[i]) {
                simulator.SetAs1(i);
            }
        }
        if (run_count > 1 && !sample_summary) {
            PrintRunHeader(std::cout, run_idx + 1, run_count);
        }

        // Run simulator
        LOG_INFO("Start simulation (run {}).", run_idx + 1);
        simulator.RunAll();
        LOG_INFO("Finish simulation (run {}).", run_idx + 1);

        // Print results
        LOG_INFO("Print run results (run {}).", run_idx + 1);
        if (parsed_state == runtime::SimulatorConfig::StateType::FullQuantum) {
            const auto run_samples = simulator.GetFullQuantumState().Sampling(1);
            all_samples.insert(all_samples.end(), run_samples.begin(), run_samples.end());
            if (sample_summary) {
                continue;
            }
            PrintConfigSummary(
                    std::cout,
                    func,
                    config,
                    init_bits,
                    simulator.GetState(),
                    print_raw,
                    run_count
            );
            PrintFullQuantumState(
                    std::cout,
                    func,
                    simulator.GetFullQuantumState(),
                    simulator.GetState(),
                    1,
                    print_raw
            );
        } else if (parsed_state == runtime::SimulatorConfig::StateType::Toffoli) {
            PrintConfigSummary(
                    std::cout,
                    func,
                    config,
                    init_bits,
                    simulator.GetState(),
                    print_raw,
                    run_count
            );
            PrintToffoliState(std::cout, func, simulator.GetToffoliState(), simulator.GetState());
        } else {
            std::cerr << "unsupported state type: " << FormatModelLabel(parsed_state) << std::endl;
            return ReturnStatus::Failure;
        }
    }

    if (sample_summary) {
        PrintSection(std::cout, "Samples Summary");
        PrintConfigLine(std::cout, "function", func->GetName());
        PrintConfigLine(std::cout, "num_runs", std::to_string(run_count));
        PrintConfigLine(std::cout, "seed_range", fmt::format("{}~{}", seed, seed + run_count - 1));
        PrintSection(std::cout, "Summary");
        PrintSamplingSummary(std::cout, all_samples, func->GetNumQubits());
    }

    return ReturnStatus::Success;
}

ReturnStatus CommandSimulate::Main(int argc, const char** argv) {
    namespace po = boost::program_options;

    // Define description.
    // clang-format off
    auto description = po::options_description(
            R"(qret 'simulate' options
Simulate a function in an IR file.

Examples:
  qret simulate --input <ir-file> --function <name>
  qret simulate --input <ir-file> --function <name> --state Toffoli --max_superpositions 16 --init_state 0101 --num_samples 8
  qret simulate --input <ir-file> --function <name> --state FullQuantum --print_raw
)"
    );
    description.add_options()
        ("help,h", "Show this help and exit.")
        ("quiet", "Suppress non-error output.")
        ("verbose", "Enable verbose logging (more detail than default).")
        ("debug", "Enable debug logging (most detailed; implies --verbose).")
        ("color", "Enable colored output.")
        ("input,i", po::value<std::string>(), "Path of input IR file.")
        ("function,f", po::value<std::string>(), "Function name to simulate.")
        ("state,s", po::value<std::string>()->default_value("FullQuantum"),
         "Simulation model. Allowed values: FullQuantum, Toffoli. "
         "Aliases: full|fullquantum for FullQuantum, tof for Toffoli.")
        /****** Initialize options ******/
        ("init_state", po::value<std::string>()->default_value(""),
         "Initial state as binary string for all circuit qubits. "
         "The first char is q0, second is q1, ... (LSB-first). "
         "E.g. --init_state 0101 means q0=0, q1=1, q2=0, q3=1. "
         "Whitespace and '_' are ignored; 0b prefix is accepted. Empty means all zeros.")
        /****** General options ******/
        ("seed", po::value<std::uint64_t>()->default_value(1), "Seed for the first run. "
         "For repeated runs, this value is incremented by 1 each time.")
        ("num_samples,n", po::value<std::uint64_t>()->default_value(0),
         "Number of simulation runs. Seeds are changed for each run using `seed + run_index`. "
         "If omitted and --print_raw is not set for FullQuantum, defaults to 10.")
        ("sample_summary", "Output only sampling summary (FullQuantum only).")
        ("print_raw", "For FullQuantum: print expanded state vector. For Toffoli, raw state is shown.")
        /****** FullQuantum options ******/
        ("use_qulacs", "[FullQuantum] Use Qulacs backend if available.")
        /****** Toffoli options ******/
        ("max_superpositions", po::value<std::uint64_t>()->default_value(1),
         "[Toffoli] Maximum number of superpositions allowed during simulation.")
    ; // NOLINT
    // clang-format on

    auto vm = po::variables_map();
    try {
        po::store(po::parse_command_line(argc, argv, description), vm);
        po::notify(vm);
    } catch (const po::error_with_option_name& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << "To get the list of available options, run 'qret simulate --help'."
                  << std::endl;
        return ReturnStatus::Failure;
    }

    if (vm.count("help") > 0) {
        std::cout << description;
        return ReturnStatus::Success;
    }
    if (vm.count("quiet") > 0) {
        qret::Logger::DisableConsoleOutput();
        qret::Logger::DisableFileOutput();
    } else if (vm.count("debug") > 0) {
        qret::Logger::EnableConsoleOutput();
        qret::Logger::SetLogLevel(qret::LogLevel::Debug);
    } else if (vm.count("verbose") > 0) {
        qret::Logger::EnableConsoleOutput();
        qret::Logger::SetLogLevel(qret::LogLevel::Info);
    } else {
        qret::Logger::EnableConsoleOutput();
        qret::Logger::SetLogLevel(qret::LogLevel::Warn);
    }
    if (vm.count("color") > 0) {
        qret::Logger::EnableColorfulOutput();
    } else {
        qret::Logger::DisableColorfulOutput();
    }

    if (vm.count("input") == 0) {
        std::cerr << "missing required option: --input <file>" << std::endl;
        return ReturnStatus::Failure;
    }
    if (vm.count("function") == 0) {
        std::cerr << "missing required option: --function <name>" << std::endl;
        return ReturnStatus::Failure;
    }

    const auto input = vm["input"].as<std::string>();
    const auto function_name = vm["function"].as<std::string>();
    const auto state = vm["state"].as<std::string>();
    const auto init_state = vm["init_state"].as<std::string>();
    const auto seed = vm["seed"].as<std::uint64_t>();
    const auto sample_summary = vm.count("sample_summary") > 0;
    const auto print_raw = vm.count("print_raw") > 0;
    auto num_samples = vm["num_samples"].as<std::uint64_t>();
    const auto num_samples_provided = !vm["num_samples"].defaulted();
    runtime::SimulatorConfig::StateType parsed_state;
    try {
        parsed_state = ParseState(state);
    } catch (const std::invalid_argument& ex) {
        std::cerr << ex.what() << std::endl;
        return ReturnStatus::Failure;
    }
    if (parsed_state == runtime::SimulatorConfig::StateType::FullQuantum && !print_raw
        && !num_samples_provided) {
        num_samples = 10;
    } else if (!num_samples_provided) {
        num_samples = 1;
    }
    const auto max_superpositions = vm["max_superpositions"].as<std::uint64_t>();
    const auto use_qulacs = vm.count("use_qulacs") > 0;

    return Simulate(
            input,
            function_name,
            state,
            init_state,
            seed,
            num_samples,
            sample_summary,
            print_raw,
            use_qulacs,
            max_superpositions
    );
}
}  // namespace qret::cmd
