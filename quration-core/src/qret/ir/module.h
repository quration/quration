/**
 * @file qret/ir/module.h
 * @brief Module.
 */

#ifndef QRET_IR_MODULE_H
#define QRET_IR_MODULE_H

#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "qret/base/iterator.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/qret_export.h"

namespace qret::ir {
// Forward declarations
class IRContext;

/**
 * @brief Module of qret IR.
 * @details Modules are the top level container of all other IR objects.
 */
class QRET_EXPORT Module {
public:
    using FunctionListType = std::list<std::unique_ptr<Function>>;

    Module() = delete;
    ~Module() = default;
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
    Module(Module&&) = default;
    Module& operator=(Module&&) = delete;

    static Module* Create(std::string_view name, IRContext& context);

    using iterator = ListIterator<Function, false>;
    using const_iterator = ListIterator<Function, true>;
    iterator begin() {
        return iterator{function_list_.begin()};
    }  // NOLINT
    const_iterator begin() const {
        return const_iterator{function_list_.cbegin()};
    }  // NOLINT
    iterator end() {
        return iterator{function_list_.end()};
    }  // NOLINT
    const_iterator end() const {
        return const_iterator{function_list_.cend()};
    }  // NOLINT

    IRContext& GetContext() const {
        return context_;
    }
    std::string_view GetName() const {
        return name_;
    }
    void SetName(std::string_view name) {
        name_ = name;
    }

    FunctionListType::iterator InsertFunction(std::unique_ptr<Function>&& func);
    Function* GetFunction(std::string_view name) const;
    FunctionListType::iterator InsertCircuit(std::unique_ptr<Function>&& func) {
        return InsertFunction(std::move(func));
    }
    Function* GetCircuit(std::string_view name) const {
        return GetFunction(name);
    }

private:
    friend Function* Function::Create(std::string_view name, Module* parent);
    friend QRET_EXPORT std::unique_ptr<Function> RemoveFromParent(Function* func);

    Module(std::string_view name, IRContext& context);

    IRContext& context_;
    std::string name_;
    FunctionListType function_list_;
    std::unordered_map<const Function*, FunctionListType::iterator> ptr2itr_;
};
}  // namespace qret::ir

#endif  // QRET_IR_MODULE_H
