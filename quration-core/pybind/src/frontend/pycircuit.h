#pragma once

#include <nanobind/nanobind.h>

#include <optional>
#include <string_view>

#include "qret/codegen/machine_function.h"
#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/ir/function.h"

namespace pyqret {
namespace frontend = qret::frontend;
namespace ir = qret::ir;

struct PyArgInfo {
    std::string arg_name;
    std::size_t size;
    frontend::Circuit::Attribute attribute;
    bool is_array;

    frontend::Circuit::Type GetType() const {
        switch (attribute) {
            case frontend::Circuit::Attribute::Operate:
            case frontend::Circuit::Attribute::CleanAncilla:
            case frontend::Circuit::Attribute::DirtyAncilla:
            case frontend::Circuit::Attribute::Catalyst:
                return frontend::Circuit::Type::Qubit;
            case frontend::Circuit::Attribute::Input:
            case frontend::Circuit::Attribute::Output:
                return frontend::Circuit::Type::Register;
        }
    }
    std::string Type() const {
        switch (GetType()) {
            case frontend::Circuit::Type::Qubit:
                return "Qubit";
            case frontend::Circuit::Type::Register:
                return "Register";
        }
    }
    std::string Attribute() const {
        switch (attribute) {
            case frontend::Circuit::Attribute::Operate:
                return "Operate";
            case frontend::Circuit::Attribute::CleanAncilla:
                return "CleanAncilla";
            case frontend::Circuit::Attribute::DirtyAncilla:
                return "DirtyAncilla";
            case frontend::Circuit::Attribute::Catalyst:
                return "Catalyst";
            case frontend::Circuit::Attribute::Input:
                return "Input";
            case frontend::Circuit::Attribute::Output:
                return "Output";
        }
    }
};

class PyArgument {
public:
    explicit PyArgument() = default;
    ~PyArgument() = default;

    static PyArgument FromIR(const ir::Function& func) {
        auto ret = PyArgument{};
        const auto& arg = func.GetArgument();
        for (const auto& [name, size] : arg.qubits) {
            ret.Add(name,
                    frontend::Circuit::Type::Qubit,
                    size,
                    frontend::Circuit::Attribute::Operate,
                    true);
        }
        for (const auto& [name, size] : arg.registers) {
            ret.Add(name,
                    frontend::Circuit::Type::Register,
                    size,
                    frontend::Circuit::Attribute::Output,
                    true);
        }
        return ret;
    }

    void SetBuilder(frontend::CircuitBuilder* builder) {
        builder_ = builder;
    }
    void Add(
            std::string_view arg_name,
            frontend::Circuit::Type type,
            std::size_t size,
            frontend::Circuit::Attribute attr,
            bool is_array
    ) {
        arg_.Add(arg_name, type, size, attr);
        is_array_.emplace_back(is_array);
        if (type == frontend::Circuit::Type::Qubit) {
            offset_.emplace_back(num_qubits);
            num_qubits += size;
        } else {
            offset_.emplace_back(num_registers);
            num_registers += size;
        }
    }
    std::vector<std::string> Keys() const {
        auto ret = std::vector<std::string>();
        ret.reserve(arg_.GetNumArgs());
        for (const auto& arg_info : arg_) {
            const auto& [arg_name, _type, _size, _attr] = arg_info;
            ret.emplace_back(arg_name);
        }
        return ret;
    }
    std::vector<PyArgInfo>
    GetArgs(frontend::Circuit::Type t, frontend::Circuit::Attribute a) const {
        auto ret = std::vector<PyArgInfo>();
        for (const auto& arg_info : arg_) {
            const auto& [arg_name, type, _size, attr] = arg_info;
            if (type == t && attr == a) {
                ret.emplace_back(GetArgInfo(arg_name));
            }
        }
        return ret;
    }
    std::vector<PyArgInfo> GetOperate() const {
        return GetArgs(frontend::Circuit::Type::Qubit, frontend::Circuit::Attribute::Operate);
    }
    std::vector<PyArgInfo> GetCleanAncilla() const {
        return GetArgs(frontend::Circuit::Type::Qubit, frontend::Circuit::Attribute::CleanAncilla);
    }
    std::vector<PyArgInfo> GetDirtyAncilla() const {
        return GetArgs(frontend::Circuit::Type::Qubit, frontend::Circuit::Attribute::DirtyAncilla);
    }
    std::vector<PyArgInfo> GetCatalyst() const {
        return GetArgs(frontend::Circuit::Type::Qubit, frontend::Circuit::Attribute::Catalyst);
    }
    std::vector<PyArgInfo> GetInput() const {
        return GetArgs(frontend::Circuit::Type::Register, frontend::Circuit::Attribute::Input);
    }
    std::vector<PyArgInfo> GetOutput() const {
        return GetArgs(frontend::Circuit::Type::Register, frontend::Circuit::Attribute::Output);
    }
    PyArgInfo GetArgInfo(std::string_view arg_name) const {
        const auto& [arg_info, offset, is_array] = GetImpl(arg_name);
        const auto& [ret_arg_name, type, size, attr] = arg_info;
        assert(ret_arg_name == arg_name);
        return {std::string(arg_name), size, attr, is_array};
    }
    nanobind::object PyGet(std::string_view arg_name) const {
        const auto& [arg_info, offset, is_array] = GetImpl(arg_name);
        const auto& [_, type, size, attr] = arg_info;
        switch (type) {
            case frontend::Circuit::Type::Qubit:
                if (is_array) {
                    return nanobind::cast(builder_->GetQubits(offset, size));
                } else {
                    return nanobind::cast(builder_->GetQubit(offset));
                }
            case frontend::Circuit::Type::Register:
                if (is_array) {
                    return nanobind::cast(builder_->GetRegisters(offset, size));
                } else {
                    return nanobind::cast(builder_->GetRegister(offset));
                }
        }
    }
    std::tuple<std::size_t, std::size_t> GetStartAndSize(std::string_view arg_name) const {
        auto start = std::size_t{0};
        auto size = std::size_t{0};
        const auto type = GetArgInfo(arg_name).GetType();
        for (const auto& [tmp_name, tmp_type, tmp_size, _attr] : arg_) {
            if (arg_name == tmp_name) {
                size = tmp_size;
                break;
            }
            if (type == tmp_type) {
                start += tmp_size;
            }
        }
        return {start, size};
    }
    std::size_t GetNumArgs() const {
        return arg_.GetNumArgs();
    }

private:
    std::tuple<const frontend::Circuit::Argument::ArgInfo&, std::size_t, bool> GetImpl(
            std::string_view arg_name
    ) const {
        const auto arg_idx = arg_.GetArgIdx(arg_name);
        return {arg_.GetArgInfo(arg_idx), offset_[arg_idx], is_array_[arg_idx]};
    }

    frontend::CircuitBuilder* builder_ = nullptr;
    std::size_t num_qubits = 0;
    std::size_t num_registers = 0;
    frontend::Circuit::Argument arg_;
    std::vector<bool> is_array_;
    std::vector<std::size_t> offset_;
};

struct PyCircuit {
    PyArgument argument;

    frontend::Circuit* frontend = nullptr;
    ir::Function* ir = nullptr;
    std::optional<qret::MachineFunction> mf;

    bool HasFrontend() const {
        return frontend != nullptr;
    }
    frontend::Circuit& GetFrontend() {
        return *frontend;
    }
    const frontend::Circuit& GetFrontend() const {
        return *frontend;
    }
    bool HasIR() const {
        return ir != nullptr;
    }
    ir::Function& GetIR() {
        return *ir;
    }
    const ir::Function& GetIR() const {
        return *ir;
    }
    bool HasMF() const {
        return mf.has_value();
    }
    qret::MachineFunction& GetMF() {
        return mf.value();
    }
    const qret::MachineFunction& GetMF() const {
        return mf.value();
    }
};

void BindPyCircuit(nanobind::module_& m);
}  // namespace pyqret
