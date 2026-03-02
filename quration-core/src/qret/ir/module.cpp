/**
 * @file qret/ir/module.cpp
 * @brief Module.
 */

#include "qret/ir/module.h"

#include <algorithm>
#include <memory>
#include <string_view>

#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"

namespace qret::ir {
Module* Module::Create(std::string_view name, IRContext& context) {
    context.owned_module.emplace_back(std::unique_ptr<Module>(new Module(name, context)));
    return context.owned_module.back().get();
}
Module::FunctionListType::iterator Module::InsertFunction(std::unique_ptr<Function>&& func) {
    function_list_.push_back(std::move(func));
    ptr2itr_.emplace(func.get(), std::prev(function_list_.end()));
    return std::prev(function_list_.end());
}
Function* Module::GetFunction(std::string_view name) const {
    // TODO: Get func in constant order.
    auto itr = std::find_if(function_list_.begin(), function_list_.end(), [name](const auto& func) {
        return func->GetName() == name;
    });
    return itr == function_list_.end() ? nullptr : itr->get();
}
Module::Module(std::string_view name, IRContext& context)
    : context_{context}
    , name_{name} {}
}  // namespace qret::ir
