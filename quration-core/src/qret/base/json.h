/**
 * @file qret/base/json.h
 * @brief Json serializer of base module.
 * @details This file defines serialize/deserialize functions of classes associated with base
 * module.
 */

#ifndef QRET_BASE_JSON_H
#define QRET_BASE_JSON_H

#include <nlohmann/json.hpp>

#include "qret/base/portable_function.h"
#include "qret/qret_export.h"

namespace qret {
using Json = nlohmann::ordered_json;

// qret/base/portable_function.h

QRET_EXPORT void to_json(Json& j, const PortableFunction::RegisterType& register_type);  // NOLINT
QRET_EXPORT void from_json(const Json& j, PortableFunction::RegisterType& register_type);  // NOLINT
QRET_EXPORT void to_json(Json& j, const PortableFunction::RegisterAccessor& accessor);  // NOLINT
QRET_EXPORT void from_json(const Json& j, PortableFunction::RegisterAccessor& accessor);  // NOLINT
QRET_EXPORT void to_json(Json& j, const PortableFunction& function);  // NOLINT
QRET_EXPORT void from_json(const Json& j, PortableFunction& function);  // NOLINT
}  // namespace qret

#endif  // QRET_BASE_JSON_H
