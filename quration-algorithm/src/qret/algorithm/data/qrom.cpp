/**
 * @file qret/algorithm/data/qrom.cpp
 * @brief QROM, QROAM, and parallel QROM.
 */

#include "qret/algorithm/data/qrom.h"

#include <stdexcept>
#include <vector>

#include "qret/algorithm/arithmetic/boolean.h"
#include "qret/algorithm/util/array.h"
#include "qret/algorithm/util/util.h"
#include "qret/frontend/argument.h"
#include "qret/frontend/attribute.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/control_flow.h"
#include "qret/frontend/functor.h"
#include "qret/frontend/intrinsic.h"
#include "qret/ir/instructions.h"

namespace qret::frontend::gate {
namespace {
PortableAnnotatedFunction ComputeFixupTableImm(
        std::string_view name,
        const std::size_t address_size,
        const std::size_t value_size,
        std::span<const BigInt> tmp_dict
) {
    // * input.size() == value_size
    //     1. measurement results : size = value_size
    // * output.size() == 2^(address_size-1) * 2
    //     1. fixup table: size = 2 * 2^(address_size-1)
    auto builder = PortableFunctionBuilder();
    auto input = builder.AddBoolArrayInputVariable("input", value_size);
    auto output = builder.AddBoolArrayOutputVariable(
            "output",
            (std::size_t{1} << (address_size - 1)) * 2
    );

    const auto dict_size = tmp_dict.size();
    const auto dict = [&tmp_dict](const auto address, const auto i) -> bool {
        return static_cast<bool>((tmp_dict[address] >> i) & 1);
    };
    const auto result = [&input](const auto i) { return input[i]; };
    const auto need_phase_fix = [&](const auto address) {
        auto need_fix = (builder.AddTmpBoolVariable() = false);
        if (address >= dict_size) {
            return need_fix;
        }
        for (auto i = std::size_t{0}; i < value_size; ++i) {
            need_fix = builder.Mux(dict(address, i) & result(i), !need_fix, need_fix);
        }
        return need_fix;
    };
    for (auto i = std::size_t{0}; i < (std::size_t{1} << (address_size - 1)); ++i) {
        const auto f1 = need_phase_fix(2 * i);
        const auto f2 = need_phase_fix((2 * i) + 1);
        const auto val =
                2 * (builder.AddTmpIntVariable() = f2) + (builder.AddTmpIntVariable() = f1);
        output[2 * i] = val >> 1 & 1;
        output[(2 * i) + 1] = val & 1;
    }
    return PortableAnnotatedFunction(builder.Compile(name));
}
PortableAnnotatedFunction ComputeFixupTable(
        std::string_view name,
        const std::size_t address_size,
        const std::size_t value_size,
        const std::size_t dict_size
) {
    // * input.size() == dict_size * value_size + value_size
    //     1. original dictionary : size = dict_size * value_size
    //     2. measurement results : size = value_size
    // * output.size() == 2^(address_size-1) * 2
    //     1. fixup table: size = 2 * 2^(address_size-1)
    auto builder = PortableFunctionBuilder();
    auto input = builder.AddBoolArrayInputVariable("input", (dict_size * value_size) + value_size);
    auto output = builder.AddBoolArrayOutputVariable(
            "output",
            (std::size_t{1} << (address_size - 1)) * 2
    );

    const auto offset = dict_size * value_size;
    const auto dict = [&input, value_size](const auto address, const auto i) {
        assert(i < value_size);
        return input[address * value_size + i];
    };
    const auto result = [&input, offset](const auto i) { return input[offset + i]; };
    const auto need_phase_fix = [&](const auto address) {
        auto need_fix = (builder.AddTmpBoolVariable() = false);
        if (address >= dict_size) {
            return need_fix;
        }
        for (auto i = std::size_t{0}; i < value_size; ++i) {
            need_fix = builder.Mux(dict(address, i) & result(i), !need_fix, need_fix);
        }
        return need_fix;
    };
    for (auto i = std::size_t{0}; i < (std::size_t{1} << (address_size - 1)); ++i) {
        const auto f1 = need_phase_fix(2 * i);
        const auto f2 = need_phase_fix((2 * i) + 1);
        const auto val =
                2 * (builder.AddTmpIntVariable() = f2) + (builder.AddTmpIntVariable() = f1);
        output[2 * i] = val >> 1 & 1;
        output[(2 * i) + 1] = val & 1;
    }
    return PortableAnnotatedFunction(builder.Compile(name));
}
PortableAnnotatedFunction ComputeFixupTableForQROAM(
        std::string_view name,
        const std::size_t address_size,
        const std::size_t value_size,
        const std::size_t dict_size
) {
    // * input.size() == dict_size * value_size + value_size
    //     1. original dictionary: size = dict_size * value_size
    //     2. measurement results: size = value_size
    // * output.size() == 2^address_width
    //     1. fixup table: size = 2^address_size
    auto builder = PortableFunctionBuilder();
    auto input = builder.AddBoolArrayInputVariable("input", (dict_size * value_size) + value_size);
    auto output = builder.AddBoolArrayOutputVariable("output", std::size_t{1} << address_size);

    const auto offset = dict_size * value_size;
    const auto dict = [&input, value_size](const auto address, const auto i) {
        assert(i < value_size);
        return input[address * value_size + i];
    };
    const auto result = [&input, offset](const auto i) { return input[offset + i]; };
    const auto need_phase_fix = [&](const auto address) {
        auto need_fix = (builder.AddTmpBoolVariable() = false);
        if (address >= dict_size) {
            return need_fix;
        }
        for (auto i = std::size_t{0}; i < value_size; ++i) {
            need_fix = builder.Mux(dict(address, i) & result(i), !need_fix, need_fix);
        }
        return need_fix;
    };
    for (auto i = std::size_t{0}; i < std::size_t{1} << address_size; ++i) {
        output[i] = need_phase_fix(i);
    }
    return PortableAnnotatedFunction(builder.Compile(name));
}
struct QRET_EXPORT UnitaryMoveGen : CircuitGenerator {
    std::size_t n;
    std::size_t m;

    static inline const char* Name = "UnitaryMove";
    explicit UnitaryMoveGen(CircuitBuilder* builder, std::size_t n, std::size_t m)
        : CircuitGenerator(builder)
        , n{n}
        , m{m} {}
    std::string GetName() const override {
        return Name;
    }
    std::string GetCacheKey() const override {
        return fmt::format("{}({},{})", GetName(), n, m);
    }
    void SetArgument(Argument& arg) const override {
        arg.Add("address", Type::Qubit, n, Attribute::Operate);
        arg.Add("dst", Type::Qubit, m * ((std::size_t{1} << n) - 1), Attribute::Operate);
        arg.Add("src", Type::Qubit, m, Attribute::Operate);
        arg.Add("aux", Type::Qubit, 1, Attribute::CleanAncilla);
    }
    Circuit* Generate() const override {
        if (IsCached()) {
            return GetCachedCircuit();
        }
        BeginCircuitDefinition();
        auto address = GetQubits(0);  // size = n
        auto dst = GetQubits(1);  // size = m * (2^n - 1)
        auto src = GetQubits(2);  // size = m
        auto aux = GetQubit(3);

        const auto num_duplicates = (std::size_t{1} << n) - 1;

        auto dst_impl = std::vector<Qubits>();
        dst_impl.reserve(num_duplicates + 1);
        for (auto i = std::size_t{0}; i < num_duplicates + 1; ++i) {
            if (i == std::size_t{0}) {
                dst_impl.emplace_back(src);
            } else {
                dst_impl.emplace_back(dst.Range((i - std::size_t{1}) * m, m));
            }
        }

        for (auto j = std::size_t{0}; j < n; ++j) {
            const auto i = n - 1 - j;
            const auto ctrl = address[i];
            const auto diff = std::size_t{1} << i;
            for (auto k = std::size_t{0}; k < (std::size_t{1} << j); ++k) {
                const auto lhs = 2 * diff * k;
                const auto rhs = lhs + diff;
                for (auto l = std::size_t{0}; l < m; ++l) {
                    ControlledSwap(ctrl, dst_impl[lhs][l], dst_impl[rhs][l], aux);
                }
            }
        }

        return EndCircuitDefinition();
    }
};
ir::CallInst*
UnitaryMove(const Qubits& address, const Qubits& dst, const Qubits& src, const CleanAncilla& aux) {
    auto gen = UnitaryMoveGen(address.GetBuilder(), address.Size(), src.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(address, dst, src, aux);
}
}  // namespace

//--------------------------------------------------//
// Functions
//--------------------------------------------------//

ir::CallInst* UnaryIterBegin(const BigInt& begin, const Qubits& address, const Qubits& aux) {
    auto gen = UnaryIterBeginGen(address.GetBuilder(), begin, address.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(address, aux);
}
ir::CallInst* UnaryIterStep(const BigInt& step, const Qubits& address, const Qubits& aux) {
    auto gen = UnaryIterStepGen(address.GetBuilder(), step, address.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(address, aux);
}
ir::CallInst* UnaryIterEnd(const BigInt& end, const Qubits& address, const Qubits& aux) {
    auto gen = UnaryIterEndGen(address.GetBuilder(), end, address.Size());
    auto* circuit = gen.Generate();
    return (*circuit)(address, aux);
}
ir::CallInst* MultiControlledUnaryIterBegin(
        const BigInt& begin,
        const Qubits& cs,
        const Qubits& address,
        const Qubits& aux
) {
    const auto c = cs.Size();
    const auto n = address.Size();
    const auto ctrl_offset = (BigInt(1) << (n + c)) - (BigInt(1) << n);
    auto gen = UnaryIterBeginGen(address.GetBuilder(), ctrl_offset + begin, n + c);
    auto* circuit = gen.Generate();
    return (*circuit)(address + cs, aux);
}
ir::CallInst* MultiControlledUnaryIterStep(
        const BigInt& step,
        const Qubits& cs,
        const Qubits& address,
        const Qubits& aux
) {
    const auto c = cs.Size();
    const auto n = address.Size();
    const auto ctrl_offset = (BigInt(1) << (n + c)) - (BigInt(1) << n);
    auto gen = UnaryIterStepGen(address.GetBuilder(), ctrl_offset + step, n + c);
    auto* circuit = gen.Generate();
    return (*circuit)(address + cs, aux);
}
ir::CallInst* MultiControlledUnaryIterEnd(
        const BigInt& end,
        const Qubits& cs,
        const Qubits& address,
        const Qubits& aux
) {
    const auto c = cs.Size();
    const auto n = address.Size();
    const auto ctrl_offset = (BigInt(1) << (n + c)) - (BigInt(1) << n);
    auto gen = UnaryIterEndGen(address.GetBuilder(), ctrl_offset + end, n + c);
    auto* circuit = gen.Generate();
    return (*circuit)(address + cs, aux);
}
ir::CallInst*
QROM(const Qubits& address, const Qubits& value, const CleanAncillae& aux, const Registers& dict) {
    const auto m = value.Size();
    if (dict.Size() % m != 0) {
        throw std::runtime_error("size of dict must be a multiplier of size of value");
    }
    const auto dict_size = dict.Size() / m;
    auto gen = MultiControlledQROMGen(
            address.GetBuilder(),
            0,
            address.Size(),
            value.Size(),
            dict_size
    );
    auto* circuit = gen.Generate();
    return (*circuit)(address.Range(0, 0), address, value, aux, dict);
}
ir::CallInst* MultiControlledQROM(
        const Qubits& cs,
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        const Registers& dict
) {
    const auto m = value.Size();
    if (dict.Size() % m != 0) {
        throw std::runtime_error("size of dict must be a multiplier of size of value");
    }
    const auto dict_size = dict.Size() / m;
    auto gen = MultiControlledQROMGen(
            address.GetBuilder(),
            cs.Size(),
            address.Size(),
            value.Size(),
            dict_size
    );
    auto* circuit = gen.Generate();
    return (*circuit)(cs, address, value, aux, dict);
}
ir::CallInst* QROMImm(
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        std::span<const BigInt> dict
) {
    auto gen =
            MultiControlledQROMImmGen(address.GetBuilder(), 0, address.Size(), value.Size(), dict);
    auto* circuit = gen.Generate();
    return (*circuit)(address.Range(0, 0), address, value, aux);
}
ir::CallInst* MultiControlledQROMImm(
        const Qubits& cs,
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        std::span<const BigInt> dict
) {
    auto gen = MultiControlledQROMImmGen(
            address.GetBuilder(),
            cs.Size(),
            address.Size(),
            value.Size(),
            dict
    );
    auto* circuit = gen.Generate();
    return (*circuit)(cs, address, value, aux);
}
ir::CallInst* UncomputeQROM(
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        const Registers& dict
) {
    const auto m = value.Size();
    if (dict.Size() % m != 0) {
        throw std::runtime_error("size of dict must be a multiplier of size of value");
    }
    const auto dict_size = dict.Size() / m;
    auto gen = UncomputeQROMGen(address.GetBuilder(), address.Size(), value.Size(), dict_size);
    auto* circuit = gen.Generate();
    return (*circuit)(address, value, aux, dict);
}
ir::CallInst* UncomputeQROMImm(
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& aux,
        std::span<const BigInt> dict
) {
    auto gen = UncomputeQROMImmGen(address.GetBuilder(), address.Size(), value.Size(), dict);
    auto* circuit = gen.Generate();
    return (*circuit)(address, value, aux);
}
ir::CallInst* QROAMClean(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const Qubits& duplicate,
        const CleanAncillae& aux,
        const Registers& dict
) {
    const auto m = value.Size();
    if (dict.Size() % m != 0) {
        throw std::runtime_error("size of dict must be a multiplier of size of value");
    }
    const auto dict_size = dict.Size() / m;
    auto gen =
            QROAMCleanGen(address.GetBuilder(), num_par, address.Size(), value.Size(), dict_size);
    auto* circuit = gen.Generate();
    return (*circuit)(address, value, duplicate, aux, dict);
}
ir::CallInst* UncomputeQROAMClean(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const Qubits& duplicate,
        const CleanAncillae& aux,
        const Registers& dict
) {
    const auto m = value.Size();
    if (dict.Size() % m != 0) {
        throw std::runtime_error("size of dict must be a multiplier of size of value");
    }
    const auto dict_size = dict.Size() / m;
    auto gen = UncomputeQROAMCleanGen(
            address.GetBuilder(),
            num_par,
            address.Size(),
            value.Size(),
            dict_size
    );
    auto* circuit = gen.Generate();
    return (*circuit)(address, value, duplicate, aux, dict);
}
ir::CallInst* QROAMDirty(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const DirtyAncillae& duplicate,
        const CleanAncillae& aux,
        const Registers& dict
) {
    const auto m = value.Size();
    if (dict.Size() % m != 0) {
        throw std::runtime_error("size of dict must be a multiplier of size of value");
    }
    const auto dict_size = dict.Size() / m;
    auto gen =
            QROAMDirtyGen(address.GetBuilder(), num_par, address.Size(), value.Size(), dict_size);
    auto* circuit = gen.Generate();
    return (*circuit)(address, value, duplicate, aux, dict);
}
ir::CallInst* UncomputeQROAMDirty(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const DirtyAncillae& duplicate,
        const CleanAncilla& src,
        const CleanAncillae& aux,
        const Registers& dict
) {
    const auto m = value.Size();
    if (dict.Size() % m != 0) {
        throw std::runtime_error("size of dict must be a multiplier of size of value");
    }
    const auto dict_size = dict.Size() / m;
    auto gen = UncomputeQROAMDirtyGen(
            address.GetBuilder(),
            num_par,
            address.Size(),
            value.Size(),
            dict_size
    );
    auto* circuit = gen.Generate();
    return (*circuit)(address, value, duplicate, src, aux, dict);
}
ir::CallInst* QROMParallel(
        std::size_t num_par,
        const Qubits& address,
        const Qubits& value,
        const CleanAncillae& duplicate_address,
        const CleanAncillae& duplicate_aux,
        const Registers& dict
) {
    const auto m = value.Size();
    if (dict.Size() % m != 0) {
        throw std::runtime_error("size of dict must be a multiplier of size of value");
    }
    const auto dict_size = dict.Size() / m;
    auto gen =
            QROMParallelGen(address.GetBuilder(), num_par, address.Size(), value.Size(), dict_size);
    auto* circuit = gen.Generate();
    return (*circuit)(address, value, duplicate_address, duplicate_aux, dict);
}

//--------------------------------------------------//
// Classes
//--------------------------------------------------//

Circuit* UnaryIterBeginGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto address = GetQubits(0);  // size = n
    auto aux = GetQubits(1);  // size = n-1

    // do Pauli-X to the i-th address qubit
    const auto do_x = [this](const std::size_t i) { return ((begin >> i) & 1) == 0; };

    for (auto i = std::size_t{0}; i < n; ++i) {
        if (do_x(i)) {
            X(address[i]);
        }
    }
    // from the most significant bit to the least significant bit
    TemporalAnd(aux[n - 2], address[n - 1], address[n - 2]);
    for (auto j = std::size_t{0}; j < n - 2; ++j) {
        const auto i = n - 3 - j;
        TemporalAnd(aux[i], address[i], aux[i + 1]);
    }
    for (auto i = std::size_t{0}; i < n; ++i) {
        if (do_x(i)) {
            X(address[i]);
        }
    }

    return EndCircuitDefinition();
}
Circuit* UnaryIterStepGen::Generate() const {
    constexpr auto UseSawtoothOptimization = true;
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    if (UseSawtoothOptimization) {
        GenerateSawtooth();
    } else {
        GenerateNaive();
    }

    return EndCircuitDefinition();
}
void UnaryIterStepGen::GenerateNaive() const {
    auto address = GetQubits(0);  // size = n
    auto aux = GetQubits(1);  // size = n-1

    // do Pauli-X to the i-th address qubit
    const auto do_x_in_uncompute = [this](const std::size_t i) { return ((step >> i) & 1) == 0; };
    const auto do_x_in_compute = [this](const std::size_t i) {
        return (((step + 1) >> i) & 1) == 0;
    };

    // Naive implementation (fig5 of 1805.03662)
    // 1. Uncompute `step`
    for (auto i = std::size_t{0}; i < n; ++i) {
        if (do_x_in_uncompute(i)) {
            X(address[i]);
        }
    }
    // from the least significant bit to the most significant bit
    for (auto i = std::size_t{0}; i < n - 2; ++i) {
        UncomputeTemporalAnd(aux[i], address[i], aux[i + 1]);
    }
    UncomputeTemporalAnd(aux[n - 2], address[n - 1], address[n - 2]);
    for (auto i = std::size_t{0}; i < n; ++i) {
        if (do_x_in_uncompute(i)) {
            X(address[i]);
        }
    }

    // 2. Compute `step+1`
    for (auto i = std::size_t{0}; i < n; ++i) {
        if (do_x_in_compute(i)) {
            X(address[i]);
        }
    }
    // from the most significant bit to the least significant bit
    TemporalAnd(aux[n - 2], address[n - 1], address[n - 2]);
    for (auto j = std::size_t{0}; j < n - 2; ++j) {
        const auto i = n - 3 - j;
        TemporalAnd(aux[i], address[i], aux[i + 1]);
    }
    for (auto i = std::size_t{0}; i < n; ++i) {
        if (do_x_in_compute(i)) {
            X(address[i]);
        }
    }
}
void UnaryIterStepGen::GenerateSawtooth() const {
    auto address = GetQubits(0);  // size = n
    auto aux = GetQubits(1);  // size = n-1

    // do Pauli-X to the i-th address qubit
    const auto do_x_in_uncompute = [this](const std::size_t i) { return ((step >> i) & 1) == 0; };
    const auto do_x_in_compute = [this](const std::size_t i) {
        return (((step + 1) >> i) & 1) == 0;
    };

    // Sawtooth optimized implementation (fig7 of 1805.03662)
    std::size_t diff_bit = n;
    for (auto j = std::size_t{0}; j < n; ++j) {
        const auto i = n - 1 - j;
        if (do_x_in_compute(i) != do_x_in_uncompute(i)) {
            diff_bit = i;
            break;
        }
    }
    assert(diff_bit < n);

    if (diff_bit == n - 1 || diff_bit == n - 2) {
        // 1. Uncompute `step`
        for (auto i = std::size_t{0}; i < n - 2; ++i) {
            if (do_x_in_uncompute(i)) {
                X(address[i]);
            }
        }
        // from the least significant bit to the most significant bit
        for (auto i = std::size_t{0}; i < n - 2; ++i) {
            UncomputeTemporalAnd(aux[i], address[i], aux[i + 1]);
        }
        for (auto i = std::size_t{0}; i < n - 2; ++i) {
            if (do_x_in_uncompute(i)) {
                X(address[i]);
            }
        }

        // 2. optimized step
        if (diff_bit == n - 1) {
            assert(do_x_in_uncompute(n - 1) == true);
            assert(do_x_in_uncompute(n - 2) == false);
            assert(do_x_in_compute(n - 1) == false);
            assert(do_x_in_compute(n - 2) == true);
            CX(aux[n - 2], address[n - 1]);
            CX(aux[n - 2], address[n - 2]);
        } else if (diff_bit == n - 2) {
            const auto a = do_x_in_uncompute(n - 1);
#ifndef NDEBUG
            const auto b = do_x_in_uncompute(n - 2);
            const auto c = do_x_in_compute(n - 1);
            const auto d = do_x_in_compute(n - 2);
            assert(a == c);
            assert(b == true);
            assert(d == false);
#endif
            if (a) {
                X(address[n - 1]);
                CX(aux[n - 2], address[n - 1]);
                X(address[n - 1]);
            } else {
                CX(aux[n - 2], address[n - 1]);
            }
        }

        // 3. Compute `step+1`
        for (auto i = std::size_t{0}; i < n - 2; ++i) {
            if (do_x_in_compute(i)) {
                X(address[i]);
            }
        }
        // from the most significant bit to the least significant bit
        for (auto j = std::size_t{0}; j < n - 2; ++j) {
            const auto i = n - 3 - j;
            TemporalAnd(aux[i], address[i], aux[i + 1]);
        }
        for (auto i = std::size_t{0}; i < n - 2; ++i) {
            if (do_x_in_compute(i)) {
                X(address[i]);
            }
        }
        return;
    }
    assert(diff_bit < n - 2);

    for (auto i = std::size_t{0}; i < diff_bit; ++i) {
        if (do_x_in_uncompute(i)) {
            X(address[i]);
        }
    }
    for (auto i = std::size_t{0}; i < diff_bit; ++i) {
        UncomputeTemporalAnd(aux[i], address[i], aux[i + 1]);
    }
    for (auto i = std::size_t{0}; i < diff_bit; ++i) {
        if (do_x_in_uncompute(i)) {
            X(address[i]);
        }
    }

    CX(aux[diff_bit], aux[diff_bit + 1]);  // optimized step

    for (auto i = std::size_t{0}; i < diff_bit; ++i) {
        if (do_x_in_compute(i)) {
            X(address[i]);
        }
    }
    for (auto j = std::size_t{0}; j < diff_bit; ++j) {
        const auto i = diff_bit - 1 - j;
        TemporalAnd(aux[i], address[i], aux[i + 1]);
    }
    for (auto i = std::size_t{0}; i < diff_bit; ++i) {
        if (do_x_in_compute(i)) {
            X(address[i]);
        }
    }
}
Circuit* UnaryIterEndGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto address = GetQubits(0);  // size = n
    auto aux = GetQubits(1);  // size = n-1

    // do Pauli-X to the i-th address qubit
    const auto do_x = [this](const std::size_t i) { return ((end >> i) & 1) == 0; };

    for (auto i = std::size_t{0}; i < n; ++i) {
        if (do_x(i)) {
            X(address[i]);
        }
    }
    // from the least significant bit to the most significant bit
    for (auto i = std::size_t{0}; i < n - 2; ++i) {
        UncomputeTemporalAnd(aux[i], address[i], aux[i + 1]);
    }
    UncomputeTemporalAnd(aux[n - 2], address[n - 1], address[n - 2]);
    for (auto i = std::size_t{0}; i < n; ++i) {
        if (do_x(i)) {
            X(address[i]);
        }
    }

    return EndCircuitDefinition();
}
Circuit* MultiControlledQROMGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    if (c == 0 && n == 1) {
        GenerateN1();
        return EndCircuitDefinition();
    }

    auto cs = GetQubits(0);  // size = c
    auto address = GetQubits(1);  // size = n
    auto value = GetQubits(2);  // size = m
    auto aux = GetQubits(3);  // size = n + c - 1
    auto dict = GetRegisters(4);  // size = dict_size * m

    MultiControlledUnaryIterBegin(0, cs, address, aux);
    for (auto i = std::size_t{0}; i < dict_size - 1; ++i) {
        ApplyCXToEachIf(aux[0], value, dict.Range(i * m, m));
        MultiControlledUnaryIterStep(i, cs, address, aux);
    }
    ApplyCXToEachIf(aux[0], value, dict.Range((dict_size - 1) * m, m));
    MultiControlledUnaryIterEnd(dict_size - 1, cs, address, aux);

    return EndCircuitDefinition();
}
void MultiControlledQROMGen::GenerateN1() const {
    auto address = GetQubit(1);  // size = n = 1
    auto value = GetQubits(2);  // size = m
    auto dict = GetRegisters(4);  // size = dict_size * m

    X(address);
    ApplyCXToEachIf(address, value, dict.Range(0, m));
    X(address);
    if (dict_size == 2) {
        ApplyCXToEachIf(address, value, dict.Range(0, m));
    }
}
Circuit* MultiControlledQROMImmGen::Generate() const {
    BeginCircuitDefinition();

    if (c == 0 && n == 1) {
        GenerateN1();
        return EndCircuitDefinition();
    }

    auto cs = GetQubits(0);  // size = c
    auto address = GetQubits(1);  // size = n
    auto value = GetQubits(2);  // size = m
    auto aux = GetQubits(3);  // size = n + c - 1

    const auto dict_size = dict.size();

    MultiControlledUnaryIterBegin(0, cs, address, aux);
    for (auto i = std::size_t{0}; i < dict_size - 1; ++i) {
        ApplyCXToEachIfImm(dict[i], aux[0], value);
        MultiControlledUnaryIterStep(i, cs, address, aux);
    }
    ApplyCXToEachIfImm(dict[dict_size - 1], aux[0], value);
    MultiControlledUnaryIterEnd(dict_size - 1, cs, address, aux);

    return EndCircuitDefinition();
}
void MultiControlledQROMImmGen::GenerateN1() const {
    const auto dict_size = dict.size();
    auto address = GetQubit(1);  // size = n = 1
    auto value = GetQubits(2);  // size = m

    X(address);
    ApplyCXToEachIfImm(dict[0], address, value);
    X(address);
    if (dict_size == 2) {
        ApplyCXToEachIfImm(dict[1], address, value);
    }
}
Circuit* UncomputeQROMGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();

    if (n == 1) {
        GenerateN1();
        return EndCircuitDefinition();
    }

    auto address = GetQubits(0);  // size = n
    auto value = GetQubits(1);  // size = m
    auto aux = GetQubits(2);  // size = n-1
    auto dict = GetRegisters(3);  // size = dict_size * m

    auto out = GetTemporalRegisters(m);
    auto fixup = GetTemporalRegisters(2 * (std::size_t{1} << (n - 1)));

    // 1. Preprocess
    auto lo_address = address[0];
    auto hi_address = address.Range(1, n - 1);
    auto lo_aux = aux[0];
    auto hi_aux = aux.Range(1, n - 2);
    X(lo_aux);
    CX(lo_aux, lo_address);
    H(lo_aux);
    H(lo_address);

    // 2. Measure `value` in Pauli-X axis
    for (auto i = std::size_t{0}; i < m; ++i) {
        H(value[i]);
        Measure(value[i], out[i]);
        control_flow::If(out[i]);
        { X(value[i]); }
        control_flow::EndIf(out[i]);
        MARK_AS_CLEAN(value[i]);
    }

    // 3. Compute fixup table
    CallCF(ComputeFixupTable("ComputeFixupTable", n, m, dict_size), dict + out, fixup);

    // 4. QROM
    QROM(hi_address, lo_address + lo_aux, hi_aux, fixup);

    // 5. Postprocess
    H(lo_aux);
    H(lo_address);
    CX(lo_aux, lo_address);
    X(lo_aux);

    return EndCircuitDefinition();
}
void UncomputeQROMGen::GenerateN1() const {
    auto address = GetQubits(0);  // size = n
    auto value = GetQubits(1);  // size = m
    auto aux = GetQubits(2);  // size = n-1
    auto dict = GetRegisters(3);  // size = dict_size * m
    QROM(address, value, aux, dict);
}
Circuit* UncomputeQROMImmGen::Generate() const {
    BeginCircuitDefinition();

    if (n == 1) {
        GenerateN1();
        return EndCircuitDefinition();
    }

    auto address = GetQubits(0);  // size = n
    auto value = GetQubits(1);  // size = m
    auto aux = GetQubits(2);  // size = n-1

    auto out = GetTemporalRegisters(m);
    auto fixup = GetTemporalRegisters(2 * (std::size_t{1} << (n - 1)));

    // 1. Preprocess
    auto lo_address = address[0];
    auto hi_address = address.Range(1, n - 1);
    auto lo_aux = aux[0];
    auto hi_aux = aux.Range(1, n - 2);
    X(lo_aux);
    CX(lo_aux, lo_address);
    H(lo_aux);
    H(lo_address);

    // 2. Measure `value` in Pauli-X axis
    for (auto i = std::size_t{0}; i < m; ++i) {
        H(value[i]);
        Measure(value[i], out[i]);
        control_flow::If(out[i]);
        X(value[i]);
        control_flow::EndIf(out[i]);
        MARK_AS_CLEAN(value[i]);
    }

    // 3. Compute fixup table
    CallCF(ComputeFixupTableImm("ComputeFixupTableImm", n, m, dict), out, fixup);

    // 4. QROM
    QROM(hi_address, lo_address + lo_aux, hi_aux, fixup);

    // 5. Postprocess
    H(lo_aux);
    H(lo_address);
    CX(lo_aux, lo_address);
    X(lo_aux);

    return EndCircuitDefinition();
}
void UncomputeQROMImmGen::GenerateN1() const {
    auto address = GetQubits(0);  // size = n
    auto value = GetQubits(1);  // size = m
    auto aux = GetQubits(2);  // size = n-1
    QROMImm(address, value, aux, dict);
}
Circuit* QROAMCleanGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto address = GetQubits(0);  // size = n
    auto value = GetQubits(1);  // size = m
    auto duplicate = GetQubits(2);  // size =  (2^num_par - 1) * m
    auto aux = GetQubits(3);  // size = n - num_par - 1
    auto dict = GetRegisters(4);  // size = dict_size * m

    const auto num_threads = std::size_t{1} << num_par;
    const auto num_items_per_thread = std::size_t{1} << (n - num_par);

    auto zero = GetTemporalRegisters(m);

    auto lo_address = address.Range(0, n - num_par);
    auto hi_address = address.Range(n - num_par, num_par);

    // 1. Create extended dict
    auto dict_impl = std::vector<Registers>(num_items_per_thread, dict.Range(0, 0));
    for (auto i = std::size_t{0}; i < num_threads; ++i) {
        const auto offset = i * num_items_per_thread;
        for (auto j = std::size_t{0}; j < num_items_per_thread; ++j) {
            if (offset + j < dict_size) {
                dict_impl[j] += dict.Range((offset + j) * m, m);
            } else {
                dict_impl[j] += zero;
            }
        }
    }
    auto extended_dict = dict.Range(0, 0);
    for (const auto& v : dict_impl) {
        extended_dict += v;
    }

    // 2. T (T of fig4 in 1902.02134)
    QROM(lo_address, value + duplicate, aux, extended_dict);

    // 3. S^{dag} (S^{dag} of fig4 in 1902.02134)
    Adjoint(UnitaryMove)(hi_address, duplicate, value, aux[0]);

    return EndCircuitDefinition();
}
Circuit* UncomputeQROAMCleanGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto address = GetQubits(0);  // size = n
    auto value = GetQubits(1);  // size = m
    auto duplicate = GetQubits(2);  // size = 2^num_par
    auto aux = GetQubits(3);  // size = n - num_par - 1
    auto dict = GetRegisters(4);  // size = dict_size * m

    const auto num_threads = std::size_t{1} << num_par;
    const auto num_items_per_thread = std::size_t{1} << (n - num_par);

    auto lo_address = address.Range(0, n - num_par);
    auto hi_address = address.Range(n - num_par, num_par);

    auto out = GetTemporalRegisters(m);
    auto fixup = GetTemporalRegisters(std::size_t{1} << n);

    // 1. Measure `value` in Pauli-X axis
    for (auto i = std::size_t{0}; i < m; ++i) {
        H(value[i]);
        Measure(value[i], out[i]);
        control_flow::If(out[i]);
        X(value[i]);
        control_flow::EndIf(out[i]);
        MARK_AS_CLEAN(value[i]);
    }

    // 2. Compute fixup table
    CallCF(ComputeFixupTableForQROAM("ComputeFixupTableForQROAM", n, m, dict_size),
           dict + out,
           fixup);
    auto fixup_impl = std::vector<Registers>(num_items_per_thread, fixup.Range(0, 0));
    for (auto i = std::size_t{0}; i < num_threads; ++i) {
        const auto offset = i * num_items_per_thread;
        for (auto j = std::size_t{0}; j < num_items_per_thread; ++j) {
            fixup_impl[j] += fixup[offset + j];
        }
    }
    auto extended_fixup = fixup.Range(0, 0);
    for (const auto& v : fixup_impl) {
        extended_fixup += v;
    }

    // 3. Implement fig6 in 1902.02134
    // Preprocess: X -> S -> H
    {
        X(duplicate[0]);
        UnitaryMove(hi_address, duplicate.Range(1, num_threads - 1), duplicate[0], aux[0]);
        for (auto i = std::size_t{0}; i < num_threads; ++i) {
            H(duplicate[i]);
        }
    }
    // Lookup: T
    QROM(lo_address,
         duplicate.Range(0, num_threads),
         aux.Range(0, n - num_par - 1),
         extended_fixup);
    // Postprocess: H -> S^{dag} -> X
    {
        for (auto i = std::size_t{0}; i < num_threads; ++i) {
            H(duplicate[i]);
        }
        Adjoint(UnitaryMove)(hi_address, duplicate.Range(1, num_threads - 1), duplicate[0], aux[0]);
        X(duplicate[0]);
    }

    return EndCircuitDefinition();
}
Circuit* QROAMDirtyGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto address = GetQubits(0);  // size = n
    auto value = GetQubits(1);  // size = m
    auto duplicate = GetQubits(2);  // size =  (2^num_par - 1) * m
    auto aux = GetQubits(3);  // size = n - num_par - 1
    auto dict = GetRegisters(4);  // size = dict_size * m

    const auto num_threads = std::size_t{1} << num_par;
    const auto num_items_per_thread = std::size_t{1} << (n - num_par);

    auto zero = GetTemporalRegisters(m);

    auto lo_address = address.Range(0, n - num_par);
    auto hi_address = address.Range(n - num_par, num_par);

    // 1. Create extended dict
    auto dict_impl = std::vector<Registers>(num_items_per_thread, dict.Range(0, 0));
    for (auto i = std::size_t{0}; i < num_threads; ++i) {
        const auto offset = i * num_items_per_thread;
        for (auto j = std::size_t{0}; j < num_items_per_thread; ++j) {
            if (offset + j < dict_size) {
                dict_impl[j] += dict.Range((offset + j) * m, m);
            } else {
                dict_impl[j] += zero;
            }
        }
    }
    auto extended_dict = dict.Range(0, 0);
    for (const auto& v : dict_impl) {
        extended_dict += v;
    }

    // 2. Hadamard
    for (const auto& q : value) {
        H(q);
    }

    // 3. S -> T -> S^{dag} (S, T, S^{dag} of fig4 in 1902.02134)
    UnitaryMove(hi_address, duplicate, value, aux[0]);
    QROM(lo_address, value + duplicate, aux, extended_dict);
    Adjoint(UnitaryMove)(hi_address, duplicate, value, aux[0]);

    // 4. Hadamard
    for (const auto& q : value) {
        H(q);
    }

    // 5. S -> T -> S^{dag} (S, T, S^{dag} of fig4 in 1902.02134)
    UnitaryMove(hi_address, duplicate, value, aux[0]);
    QROM(lo_address, value + duplicate, aux, extended_dict);
    Adjoint(UnitaryMove)(hi_address, duplicate, value, aux[0]);

    return EndCircuitDefinition();
}
Circuit* UncomputeQROAMDirtyGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto address = GetQubits(0);  // size = n
    auto value = GetQubits(1);  // size = m
    auto duplicate = GetQubits(2);  // size = 2^num_par - 1
    auto src = GetQubit(3);  // size = 1
    auto aux = GetQubits(4);  // size = n - num_par - 1
    auto dict = GetRegisters(5);  // size = dict_size * m

    const auto num_threads = std::size_t{1} << num_par;
    const auto num_items_per_thread = std::size_t{1} << (n - num_par);

    auto lo_address = address.Range(0, n - num_par);
    auto hi_address = address.Range(n - num_par, num_par);

    auto out = GetTemporalRegisters(m);
    auto fixup = GetTemporalRegisters(std::size_t{1} << n);

    // 1. Measure `value` in Pauli-X axis
    for (auto i = std::size_t{0}; i < m; ++i) {
        H(value[i]);
        Measure(value[i], out[i]);
        control_flow::If(out[i]);
        X(value[i]);
        control_flow::EndIf(out[i]);
        MARK_AS_CLEAN(value[i]);
    }

    // 2. Compute fixup table
    CallCF(ComputeFixupTableForQROAM("ComputeFixupTableForQROAM", n, m, dict_size),
           dict + out,
           fixup);
    auto fixup_impl = std::vector<Registers>(num_items_per_thread, fixup.Range(0, 0));
    for (auto i = std::size_t{0}; i < num_threads; ++i) {
        const auto offset = i * num_items_per_thread;
        for (auto j = std::size_t{0}; j < num_items_per_thread; ++j) {
            fixup_impl[j] += fixup[offset + j];
        }
    }
    auto extended_fixup = fixup.Range(0, 0);
    for (const auto& v : fixup_impl) {
        extended_fixup += v;
    }

    // 3. Implement fig7 in 1902.02134
    // H -> S -> T -> S^{dag}
    {
        H(src);
        UnitaryMove(hi_address, duplicate, src, aux[0]);
        QROM(lo_address, src + duplicate, aux.Range(0, n - num_par - 1), extended_fixup);
        Adjoint(UnitaryMove)(hi_address, duplicate, src, aux[0]);
    }
    // Z -> S -> T -> S^{dag} -> Z -> H
    {
        Z(src);
        UnitaryMove(hi_address, duplicate, src, aux[0]);
        QROM(lo_address, src + duplicate, aux.Range(0, n - num_par - 1), extended_fixup);
        Adjoint(UnitaryMove)(hi_address, duplicate, src, aux[0]);
        Z(src);
        H(src);
    }

    return EndCircuitDefinition();
}
Circuit* QROMParallelGen::Generate() const {
    if (IsCached()) {
        return GetCachedCircuit();
    }
    BeginCircuitDefinition();
    auto address = GetQubits(0);  // size = n
    auto value = GetQubits(1);  // size = m
    auto duplicate_address = GetQubits(2);  // size = (2^num_par-1)*n
    auto duplicate_aux = GetQubits(3);  // size = 2^num_par * (n-1)
    auto dict = GetRegisters(4);  // size = dict_size * m
    const auto num_threads = std::size_t{1} << num_par;
    const auto num_items_per_thread = std::size_t{1} << (n - num_par);

    // Define some useful views
    auto th_address = std::vector<Qubits>(num_threads, address.Range(0, 0));
    auto th_aux = std::vector<Qubits>(num_threads, duplicate_aux.Range(0, 0));
    for (auto i = std::size_t{0}; i < num_threads; ++i) {
        if (i == std::size_t{0}) {
            th_address[i] += address;
        } else {
            th_address[i] += duplicate_address.Range((i - 1) * n, n);
        }
        th_aux[i] += duplicate_aux.Range(i * (n - 1), n - 1);
    }

    // 1. Amplify (called fanout in 2312.00696)
    {
        for (auto i = std::size_t{0}; i < num_par; ++i) {
            const auto i_pow_of_two = std::size_t{1} << i;
            for (auto j = std::size_t{0}; j < i_pow_of_two; ++j) {
                for (auto k = std::size_t{0}; k < n; ++k) {
                    CX(th_address[j + i_pow_of_two][k], th_address[j][k]);
                }
            }
        }
    }

    // 2. Parallel QROM
    for (auto i = std::size_t{0}; i < num_threads; ++i) {
        const auto offset = i * num_items_per_thread;
        const auto tmp_dict_size = std::min(num_items_per_thread, dict_size - offset);

        UnaryIterBegin(offset, th_address[i], th_aux[i]);
        for (auto j = std::size_t{0}; j < tmp_dict_size - 1; ++j) {
            ApplyCXToEachIf(th_aux[i][0], value, dict.Range((offset + j) * m, m));
            UnaryIterStep(offset + j, th_address[i], th_aux[i]);
        }
        ApplyCXToEachIf(th_aux[i][0], value, dict.Range((offset + tmp_dict_size - 1) * m, m));
        UnaryIterEnd(offset + tmp_dict_size - 1, th_address[i], th_aux[i]);

        // Early drop
        if (dict_size <= (i + 1) * num_items_per_thread) {
            break;
        }
    }

    // 3. Shrink (adjoint of Amplify)
    {
        for (auto tmp = static_cast<std::int32_t>(num_par) - 1; tmp >= 0; --tmp) {
            const auto i = static_cast<std::size_t>(tmp);
            const auto i_pow_of_two = std::size_t{1} << i;
            for (auto j = std::size_t{0}; j < i_pow_of_two; ++j) {
                for (auto k = std::size_t{0}; k < n; ++k) {
                    CX(th_address[j + i_pow_of_two][k], th_address[j][k]);
                }
            }
        }
    }

    return EndCircuitDefinition();
}
}  // namespace qret::frontend::gate
