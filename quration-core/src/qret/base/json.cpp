/**
 * @file qret/base/json.h
 * @brief Json serializer of base module.
 */

#include "qret/base/json.h"

#include <nlohmann/json.hpp>

#include "qret/base/portable_function.h"

namespace qret {
void to_json(Json& j, const PortableFunction::RegisterType& register_type) {  // NOLINT
    j["type"] = [&] {
        if (register_type.IsBool()) {
            return "Bool";
        } else if (register_type.IsBoolArray()) {
            return "BoolArray";
        } else if (register_type.IsInt()) {
            return "Int";
        } else if (register_type.IsIntArray()) {
            return "IntArray";
        }
        assert(0 && "unreachable");
        return "Unknown";
    }();
    if (register_type.IsArray()) {
        j["size"] = register_type.ArraySize();
    }
}

void from_json(const Json& j, PortableFunction::RegisterType& register_type) {  // NOLINT
    auto type = j.at("type").get<std::string>();
    if (type == "Bool") {
        register_type = PortableFunction::RegisterType::Bool();
    } else if (type == "BoolArray") {
        register_type = PortableFunction::RegisterType::BoolArray(j.at("size").get<std::size_t>());
    } else if (type == "Int") {
        register_type = PortableFunction::RegisterType::Int();
    } else if (type == "IntArray") {
        register_type = PortableFunction::RegisterType::IntArray(j.at("size").get<std::size_t>());
    } else {
        throw std::runtime_error("unknown RegisterType: " + type);
    }
}

void to_json(Json& j, const PortableFunction::RegisterAccessor& accessor) {  // NOLINT
    j["type"] = [&] {
        if (accessor.IsImmediate()) {
            return "Immediate";
        } else if (accessor.IsDirectAccess()) {
            return "DirectAccess";
        } else if (accessor.IsIndexedAccessByImmediate()) {
            return "IndexedAccessByImmediate";
        } else if (accessor.IsIndexedAccessByRegister()) {
            return "IndexedAccessByRegister";
        }
        assert(0 && "unreachable");
        return "Unknown";
    }();
    if (accessor.IsImmediate()) {
        j["value"] = accessor.GetImmediate();
    }
    if (accessor.IsDirectAccess()) {
        j["register"] = accessor.GetDirectAccess();
    }
    if (accessor.IsIndexedAccessByImmediate()) {
        auto [array_register, index_value] = accessor.GetIndexedAccessByImmediate();
        j["array_register"] = array_register;
        j["index_value"] = index_value;
    }
    if (accessor.IsIndexedAccessByRegister()) {
        auto [array_register, index_register] = accessor.GetIndexedAccessByRegister();
        j["array_register"] = array_register;
        j["index_register"] = index_register;
    }
}

void from_json(const Json& j, PortableFunction::RegisterAccessor& accessor) {  // NOLINT
    auto type = j.at("type").get<std::string>();
    if (type == "Immediate") {
        accessor = PortableFunction::RegisterAccessor(j.at("value").get<PortableFunction::Int>());
    } else if (type == "DirectAccess") {
        accessor = PortableFunction::RegisterAccessor(j.at("register").get<std::string>());
    } else if (type == "IndexedAccessByImmediate") {
        accessor = PortableFunction::RegisterAccessor(
                j.at("array_register").get<std::string>(),
                j.at("index_value").get<PortableFunction::Int>()
        );
    } else if (type == "IndexedAccessByRegister") {
        accessor = PortableFunction::RegisterAccessor(
                j.at("array_register").get<std::string>(),
                j.at("index_register").get<std::string>()
        );
    } else {
        throw std::runtime_error("unknown RegisterAccessorType: " + type);
    }
}

void to_json(Json& j, const PortableFunction& function) {  // NOLINT
    j["name"] = function.Name();
    j["input"] = function.Input();
    j["output"] = function.Output();
    j["tmp_variable"] = function.TmpVariable();
    j["instructions"] = Json::array();
    auto dumped_instructions = function.DumpInstructions();
    for (const auto& [inst_str, operands] : dumped_instructions) {
        auto inst_j = Json::object();
        inst_j["instruction"] = inst_str;
        inst_j["operands"] = operands;
        j["instructions"].push_back(inst_j);
    }
}

void from_json(const Json& j, PortableFunction& function) {  // NOLINT
    function = PortableFunction(
            j.at("name").get<std::string>(),
            j.at("input").get<std::map<std::string, PortableFunction::RegisterType>>(),
            j.at("output").get<std::map<std::string, PortableFunction::RegisterType>>(),
            j.at("tmp_variable").get<std::map<std::string, PortableFunction::RegisterType>>()
    );
    auto dumped_instructions =
            std::vector<std::tuple<std::string, std::vector<PortableFunction::RegisterAccessor>>>();
    if (j.contains("instructions")) {
        for (const auto& inst_j : j["instructions"]) {
            dumped_instructions.emplace_back(
                    inst_j.at("instruction").get<std::string>(),
                    inst_j.at("operands").get<std::vector<PortableFunction::RegisterAccessor>>()
            );
        }
    }
    function.LoadInstructions(dumped_instructions);
}
}  // namespace qret
