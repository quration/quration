/**
 * @file qret/exporter/qir.h
 * @brief Export Quration-IR to QIR (.ll text).
 */

#ifndef QRET_EXPORTER_QIR_H
#define QRET_EXPORTER_QIR_H

#include <string>
#include <string_view>

#include "qret/ir/module.h"
#include "qret/qret_export.h"

namespace qret::qir {
/**
 * @brief Export a Quration-IR module to QIR text (LLVM IR `.ll` syntax).
 * @details Always declares `qir_profiles="adaptive_profile"` (see qir.cpp). Instructions that
 * cannot be represented in QIR (MCX, DiscreteDistribution) raise qret::QRETError.
 *
 * @param module module to export
 * @param entry_function_name name of the entry function within `module`; if empty, uses the
 * module's first function
 * @return std::string QIR text
 */
QRET_EXPORT std::string
ExportModuleToQIR(const ir::Module& module, std::string_view entry_function_name = "");
}  // namespace qret::qir

#endif  // QRET_EXPORTER_QIR_H
