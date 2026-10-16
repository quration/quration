/**
 * @file qret/frontend/qualtran.h
 * @brief Qualtran BloqLibrary → Quration IR conversion.
 */

#ifndef QRET_FRONTEND_QUALTRAN_H
#define QRET_FRONTEND_QUALTRAN_H

#include <string>
#include <string_view>

#include "qret/frontend/builder.h"
#include "qret/frontend/circuit.h"
#include "qret/qret_export.h"

#include "qualtran/protos/bloq.pb.h"

namespace qret::frontend {

/**
 * @brief Build a Quration IR circuit from a parsed Qualtran BloqLibrary proto.
 *
 * @param lib        Parsed BloqLibrary protobuf message (e.g. from
 * qret::qualtran::ParseQualtranJson).
 * @param builder    CircuitBuilder backed by an existing IRContext/Module.
 * @param entry_name Name given to the generated IR circuit function.
 * @return Pointer to the newly created Circuit (owned by the Module).
 * @throws QRETError(UnsupportedQualtranBloq) for gate types not yet mapped.
 * @throws QRETError(MalformedQualtranProto)  for structurally invalid input, or for more than one
 *         root candidate (multi-root support not yet implemented).
 */
QRET_EXPORT Circuit* BuildCircuitFromBloqLibrary(
        const ::qualtran::BloqLibrary& lib,
        CircuitBuilder& builder,
        std::string_view entry_name = "main"
);

/**
 * @brief Parse a Qualtran BloqLibrary JSON string and build a Quration IR circuit.
 *
 * @param bloq_library_json  JSON text serialised from a Qualtran BloqLibrary proto.
 * @param builder            CircuitBuilder backed by an existing IRContext/Module.
 * @param entry_name         Name given to the generated IR circuit function.
 * @return Pointer to the newly created Circuit (owned by the Module).
 * @throws QRETError(UnsupportedQualtranBloq) for gate types not yet mapped.
 * @throws QRETError(MalformedQualtranProto)  for structurally invalid input.
 */
QRET_EXPORT Circuit* BuildCircuitFromQualtranJson(
        const std::string& bloq_library_json,
        CircuitBuilder& builder,
        std::string_view entry_name = "main"
);

}  // namespace qret::frontend

#endif  // QRET_FRONTEND_QUALTRAN_H
