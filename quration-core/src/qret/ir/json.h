/**
 * @file qret/ir/json.h
 * @brief Json serializer of IR module.
 */

#ifndef QRET_IR_JSON_H
#define QRET_IR_JSON_H

#include "qret/base/json.h"
#include "qret/ir/basic_block.h"
#include "qret/ir/context.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/ir/instructions.h"
#include "qret/ir/metadata.h"
#include "qret/ir/module.h"
#include "qret/ir/value.h"
#include "qret/qret_export.h"

namespace qret::ir {
inline constexpr auto IRJsonSchemaVersion = "0.1";

//--------------------------------------------------//
// Serialization
//--------------------------------------------------//

QRET_EXPORT void to_json(Json& j, const Module& module);
QRET_EXPORT void to_json(Json& j, const Function& func);
QRET_EXPORT void to_json(Json& j, const BasicBlock& bb);
QRET_EXPORT void to_json(Json& j, const Instruction& inst);
QRET_EXPORT void to_json(Json& j, const MeasurementInst& inst);
QRET_EXPORT void to_json(Json& j, const UnaryInst& inst);
QRET_EXPORT void to_json(Json& j, const ParametrizedRotationInst& inst);
QRET_EXPORT void to_json(Json& j, const BinaryInst& inst);
QRET_EXPORT void to_json(Json& j, const TernaryInst& inst);
QRET_EXPORT void to_json(Json& j, const MultiControlInst& inst);
QRET_EXPORT void to_json(Json& j, const GlobalPhaseInst& inst);
QRET_EXPORT void to_json(Json& j, const CallInst& inst);
QRET_EXPORT void to_json(Json& j, const CallCFInst& inst);
QRET_EXPORT void to_json(Json& j, const DiscreteDistInst& inst);
QRET_EXPORT void to_json(Json& j, const BranchInst& inst);
QRET_EXPORT void to_json(Json& j, const SwitchInst& inst);
QRET_EXPORT void to_json(Json& j, const ReturnInst& inst);
QRET_EXPORT void to_json(Json& j, const CleanInst& inst);
QRET_EXPORT void to_json(Json& j, const CleanProbInst& inst);
QRET_EXPORT void to_json(Json& j, const DirtyInst& inst);
QRET_EXPORT void to_json(Json& j, const FloatWithPrecision& v);
QRET_EXPORT void to_json(Json& j, const Opcode& opcode);
QRET_EXPORT void to_json(Json& j, const Metadata& metadata);
QRET_EXPORT void to_json(Json& j, const MDAnnotation& md_annot);
QRET_EXPORT void to_json(Json& j, const MDQubit& md_qubit);
QRET_EXPORT void to_json(Json& j, const MDRegister& md_register);
QRET_EXPORT void to_json(Json& j, const DIBreakPoint& di_breakpoint);
QRET_EXPORT void to_json(Json& j, const DILocation& di_location);

//--------------------------------------------------//
// Deserialization
//--------------------------------------------------//

/**
 * @brief Read the JSON serialized Module, and add the loaded Module to the IRContext.
 * @details A slightly simplified example of usage:
 * \code {.cpp}
 * Json j;
 * {
 *     Module module;
 *     // create module
 *     j = module;
 * }
 * IRContext context;
 * LoadJson(j, context);
 * \endcode
 *
 *
 * @param j json
 * @param context context
 */
QRET_EXPORT void LoadJson(const Json& j, IRContext& context);
QRET_EXPORT void LoadJsonImpl(const Json& j, Module& module);
QRET_EXPORT void from_json(const Json& j, FloatWithPrecision& v);
QRET_EXPORT void from_json(const Json& j, Opcode& opcode);

using FunctionMap = std::unordered_map<std::string, Function*>;
using CircuitMap = FunctionMap;
QRET_EXPORT void LoadFunction(const Json& j, const FunctionMap& fmap);
inline void LoadCircuit(const Json& j, const CircuitMap& cmap) {
    LoadFunction(j, cmap);
}
}  // namespace qret::ir

#endif  // QRET_IR_JSON_H
