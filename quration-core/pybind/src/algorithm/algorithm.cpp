#include "algorithm/algorithm.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/vector.h>

#include <cstddef>
#include <cstdint>
#include <utility>

#include "qret/algorithm/arithmetic/boolean.h"
#include "qret/algorithm/arithmetic/integer.h"
#include "qret/algorithm/arithmetic/modular.h"
#include "qret/algorithm/data/qdict.h"
#include "qret/algorithm/data/qrom.h"
#include "qret/algorithm/phase_estimation/qpe.h"
#include "qret/algorithm/preparation/arbitrary.h"
#include "qret/algorithm/preparation/uniform.h"
#include "qret/algorithm/transform/qft.h"
#include "qret/algorithm/util/array.h"
#include "qret/algorithm/util/bitwise.h"
#include "qret/algorithm/util/multi_control.h"
#include "qret/algorithm/util/reset.h"
#include "qret/algorithm/util/rotation.h"
#include "qret/algorithm/util/util.h"
#include "qret/algorithm/util/write_register.h"
#include "qret/base/type.h"
#include "qret/frontend/argument.h"
#include "qret/ir/value.h"

namespace pyqret {
namespace nb = nanobind;
using namespace qret::frontend;

namespace {
void BindFourierTransform(nb::module_& m) {
    m.def("fourier_transform", gate::FourierTransform, nb::rv_policy::reference);
}

void BindPrepare(nb::module_& m) {
    m.def(
            "prepare",
            [](const std::vector<std::pair<std::size_t, std::size_t>>& alias_sampling,
               const Qubits& index,
               const Qubits& alternate_index,
               const Qubits& random,
               const Qubits& switch_weight,
               const Qubit& cmp,
               const Qubits& aux) {
                auto a = std::vector<std::pair<std::uint32_t, qret::BigInt>>();
                a.reserve(alias_sampling.size());
                for (const auto& [ai, sw] : alias_sampling) {
                    a.emplace_back(ai, sw);
                }
                gate::PREPARE(a, index, alternate_index, random, switch_weight, cmp, aux);
            },
            nb::rv_policy::reference
    );
}

void BindPhaseEstimation(nb::module_& m) {
    BindPrepare(m);
}

void BindTransform(nb::module_& m) {
    BindFourierTransform(m);
}

void BindArithmetic(nb::module_& m) {
    m
            // arithmetic/boolean.h
            .def("temporal_and", gate::TemporalAnd, nb::rv_policy::reference)
            .def("uncompute_temporal_and", gate::UncomputeTemporalAnd, nb::rv_policy::reference)
            .def("rt3", gate::RT3, nb::rv_policy::reference)
            .def("irt3", gate::IRT3, nb::rv_policy::reference)
            .def("rt4", gate::RT4, nb::rv_policy::reference)
            .def("irt4", gate::IRT4, nb::rv_policy::reference)
            .def("maj", gate::MAJ, nb::rv_policy::reference)
            .def("uma", gate::UMA, nb::rv_policy::reference)

            // arithmetic/integer.h
            .def("inc", gate::Inc, nb::rv_policy::reference)
            .def("multi_controlled_inc", gate::MultiControlledInc, nb::rv_policy::reference)
            .def("add", gate::Add, nb::rv_policy::reference)
            .def("add_cuccaro", gate::AddCuccaro, nb::rv_policy::reference)
            .def("add_craig", gate::AddCraig, nb::rv_policy::reference)
            .def("controlled_add_craig", gate::ControlledAddCraig, nb::rv_policy::reference)
            .def("add_imm", gate::AddImm, nb::rv_policy::reference)
            .def("multi_controlled_add_imm", gate::MultiControlledAddImm, nb::rv_policy::reference)
            .def("add_general", gate::AddGeneral, nb::rv_policy::reference)
            .def("multi_controlled_add_general",
                 gate::MultiControlledAddGeneral,
                 nb::rv_policy::reference)
            .def("add_piecewise", gate::AddPiecewise, nb::rv_policy::reference)
            .def("sub", gate::Sub, nb::rv_policy::reference)
            .def("sub_cuccaro", gate::SubCuccaro, nb::rv_policy::reference)
            .def("sub_craig", gate::SubCraig, nb::rv_policy::reference)
            .def("controlled_sub_craig", gate::ControlledSubCraig, nb::rv_policy::reference)
            .def("sub_imm", gate::SubImm, nb::rv_policy::reference)
            .def("multi_controlled_sub_imm", gate::MultiControlledSubImm, nb::rv_policy::reference)
            .def("sub_general", gate::SubGeneral, nb::rv_policy::reference)
            .def("multi_controlled_sub_general",
                 gate::MultiControlledSubGeneral,
                 nb::rv_policy::reference)
            .def("mul_w", gate::MulW, nb::rv_policy::reference)
            .def("scaled_add", gate::ScaledAdd, nb::rv_policy::reference)
            .def("scaled_add_w", gate::ScaledAddW, nb::rv_policy::reference)
            .def("equal_to", gate::EqualTo, nb::rv_policy::reference)
            .def("equal_to_imm", gate::EqualToImm, nb::rv_policy::reference)
            .def("less_than", gate::LessThan, nb::rv_policy::reference)
            .def("less_than_or_equal_to", gate::LessThanOrEqualTo, nb::rv_policy::reference)
            .def("less_than_imm", gate::LessThanImm, nb::rv_policy::reference)
            .def("multi_controlled_less_than_imm",
                 gate::MultiControlledLessThanImm,
                 nb::rv_policy::reference)
            .def("bi_flip", gate::BiFlip, nb::rv_policy::reference)
            .def("multi_controlled_bi_flip", gate::MultiControlledBiFlip, nb::rv_policy::reference)
            .def("bi_flip_imm", gate::BiFlipImm, nb::rv_policy::reference)
            .def("multi_controlled_bi_flip_imm",
                 gate::MultiControlledBiFlipImm,
                 nb::rv_policy::reference)
            .def("pivot_flip", gate::PivotFlip, nb::rv_policy::reference)
            .def("multi_controlled_pivot_flip",
                 gate::MultiControlledPivotFlip,
                 nb::rv_policy::reference)
            .def("pivot_flip_imm", gate::PivotFlipImm, nb::rv_policy::reference)
            .def("multi_controlled_pivot_flip_imm",
                 gate::MultiControlledPivotFlipImm,
                 nb::rv_policy::reference)

            // arithmetic/modular.h
            .def("mod_neg", gate::ModNeg, nb::rv_policy::reference)
            .def("multi_controlled_mod_neg", gate::MultiControlledModNeg, nb::rv_policy::reference)
            .def("mod_add", gate::ModAdd, nb::rv_policy::reference)
            .def("multi_controlled_mod_add", gate::MultiControlledModAdd, nb::rv_policy::reference)
            .def("mod_add_imm", gate::ModAddImm, nb::rv_policy::reference)
            .def("multi_controlled_mod_add_imm",
                 gate::MultiControlledModAddImm,
                 nb::rv_policy::reference)
            .def("mod_sub", gate::ModSub, nb::rv_policy::reference)
            .def("multi_controlled_mod_sub", gate::MultiControlledModSub, nb::rv_policy::reference)
            .def("mod_sub_imm", gate::ModSubImm, nb::rv_policy::reference)
            .def("multi_controlled_mod_sub_imm",
                 gate::MultiControlledModSubImm,
                 nb::rv_policy::reference)
            .def("mod_doubling", gate::ModDoubling, nb::rv_policy::reference)
            .def("multi_controlled_mod_doubling",
                 gate::MultiControlledModDoubling,
                 nb::rv_policy::reference)
            .def("mod_bi_mul_imm", gate::ModBiMulImm, nb::rv_policy::reference)
            .def("multi_controlled_mod_bi_mul_imm",
                 gate::MultiControlledModBiMulImm,
                 nb::rv_policy::reference)
            .def("mod_scaled_add", gate::ModScaledAdd, nb::rv_policy::reference)
            .def("mod_scaled_add_w", gate::ModScaledAddW, nb::rv_policy::reference)
            .def("multi_controlled_mod_scaled_add",
                 gate::MultiControlledModScaledAdd,
                 nb::rv_policy::reference)
            .def("mod_exp_w", gate::ModExpW, nb::rv_policy::reference);
}

void BindData(nb::module_& m) {
    m
            // qdict.h
            .def("initialize_q_dict", gate::InitializeQDict, nb::rv_policy::reference)
            .def("inject_into_q_dict", gate::InjectIntoQDict, nb::rv_policy::reference)
            .def("extract_from_q_dict", gate::ExtractFromQDict, nb::rv_policy::reference)

            // qrom.h
            .def("unary_iter_begin", gate::UnaryIterBegin, nb::rv_policy::reference)
            .def("unary_iter_step", gate::UnaryIterStep, nb::rv_policy::reference)
            .def("unary_iter_end", gate::UnaryIterEnd, nb::rv_policy::reference)
            .def("multi_controlled_unary_iter_begin",
                 gate::MultiControlledUnaryIterBegin,
                 nb::rv_policy::reference)
            .def("multi_controlled_unary_iter_step",
                 gate::MultiControlledUnaryIterStep,
                 nb::rv_policy::reference)
            .def("multi_controlled_unary_iter_end",
                 gate::MultiControlledUnaryIterEnd,
                 nb::rv_policy::reference)
            .def("qrom", gate::QROM, nb::rv_policy::reference)
            .def("multi_controlled_qrom", gate::MultiControlledQROM, nb::rv_policy::reference)
            .def(
                    "qrom_imm",
                    [](const Qubits& address,
                       const Qubits& value,
                       const Qubits& aux,
                       const std::vector<qret::BigInt>& dict) {
                        gate::QROMImm(address, value, aux, dict);
                    },
                    nb::rv_policy::reference
            )
            .def(
                    "multi_controlled_qrom_imm",
                    [](const Qubits& cs,
                       const Qubits& address,
                       const Qubits& value,
                       const Qubits& aux,
                       const std::vector<qret::BigInt>& dict) {
                        gate::MultiControlledQROMImm(cs, address, value, aux, dict);
                    },
                    nb::rv_policy::reference
            )
            .def("uncompute_qrom", gate::UncomputeQROM, nb::rv_policy::reference)
            .def(
                    "uncompute_qrom_imm",
                    [](const Qubits& address,
                       const Qubits& value,
                       const Qubits& aux,
                       const std::vector<qret::BigInt>& dict) {
                        gate::UncomputeQROMImm(address, value, aux, dict);
                    },
                    nb::rv_policy::reference
            )
            .def("qroam_clean", gate::QROAMClean, nb::rv_policy::reference)
            .def("uncompute_qroam_clean", gate::UncomputeQROAMClean, nb::rv_policy::reference)
            .def("qroam_dirty", gate::QROAMDirty, nb::rv_policy::reference)
            .def("uncompute_qroam_dirty", gate::UncomputeQROAMDirty, nb::rv_policy::reference)
            .def("qrom_parallel", gate::QROMParallel, nb::rv_policy::reference);
}

void BindPreparation(nb::module_& m) {
    m
            // arbitrary.h
            .def(
                    "prepare_arbitrary_state",
                    [](const std::vector<double>& probs,
                       const Qubits& tgt,
                       const Qubits& theta,
                       const Qubits& aux) { gate::PrepareArbitraryState(probs, tgt, theta, aux); },
                    nb::rv_policy::reference
            )

            // uniform.h
            .def("prepare_uniform_superposition",
                 gate::PrepareUniformSuperposition,
                 nb::rv_policy::reference)
            .def("controlled_prepare_uniform_superposition",
                 gate::ControlledPrepareUniformSuperposition,
                 nb::rv_policy::reference);
}

void BindUtil(nb::module_& m) {
    m
            // array.h
            .def("apply_x_to_each", gate::ApplyXToEach, nb::rv_policy::reference)
            .def("apply_x_to_each_if", gate::ApplyXToEachIf, nb::rv_policy::reference)
            .def("apply_cx_to_each_if", gate::ApplyCXToEachIf, nb::rv_policy::reference)
            .def("apply_x_to_each_if_imm", gate::ApplyXToEachIfImm, nb::rv_policy::reference)
            .def("apply_cx_to_each_if_imm", gate::ApplyCXToEachIfImm, nb::rv_policy::reference)

            // bitwise.h
            .def("multi_controlled_bit_order_rotation",
                 gate::MultiControlledBitOrderRotation,
                 nb::rv_policy::reference)
            .def("multi_controlled_bit_order_reversal",
                 gate::MultiControlledBitOrderReversal,
                 nb::rv_policy::reference)

            // multi_control.h
            .def("mcmx", gate::MCMX, nb::rv_policy::reference)

            // reset.h
            .def("reset", gate::Reset, nb::rv_policy::reference)

            // rotation.h
            .def("r1", gate::R1, nb::rv_policy::reference)
            .def("controlled_r1", gate::ControlledR1, nb::rv_policy::reference)
            .def("controlled_rx", gate::ControlledRX, nb::rv_policy::reference)
            .def("controlled_ry", gate::ControlledRY, nb::rv_policy::reference)
            .def("controlled_rz", gate::ControlledRZ, nb::rv_policy::reference)
            .def(
                    "randomized_multiplexed_rotation",
                    [](std::uint64_t lookup_par,
                       std::uint64_t del_lookup_par,
                       std::uint64_t sub_bit_precision,
                       std::uint64_t seed,
                       std::vector<double>& theta,
                       const Qubit& q,
                       const Qubits& address,
                       const CleanAncillae& aux) {
                        auto t = std::vector<qret::ir::FloatWithPrecision>();
                        t.reserve(theta.size());
                        for (const auto& x : theta) {
                            t.emplace_back(x);
                        }
                        gate::RandomizedMultiplexedRotation(
                                lookup_par,
                                del_lookup_par,
                                sub_bit_precision,
                                seed,
                                t,
                                q,
                                address,
                                aux
                        );
                    },
                    nb::rv_policy::reference
            )

            // util.h
            .def("controlled_h", gate::ControlledH, nb::rv_policy::reference)
            .def("swap", gate::Swap, nb::rv_policy::reference)
            .def("controlled_swap", gate::ControlledSwap, nb::rv_policy::reference)

            // write_register.h
            .def("write_registers", gate::WriteRegisters, nb::rv_policy::reference);
}
}  // namespace

void BindAlgorithm(nb::module_& m) {
    auto m_arithmetic = m.def_submodule("arithmetic");
    auto m_data = m.def_submodule("data");
    auto m_preparation = m.def_submodule("preparation");
    auto m_util = m.def_submodule("util");
    auto m_phase_estimation = m.def_submodule("phase_estimation");
    auto m_transform = m.def_submodule("transform");

    BindArithmetic(m_arithmetic);
    BindData(m_data);
    BindPreparation(m_preparation);
    BindUtil(m_util);
    BindPhaseEstimation(m_phase_estimation);
    BindTransform(m_transform);
}
}  // namespace pyqret
