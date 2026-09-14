/**
 * @file qret/parser/qualtran.h
 * @brief Qualtran BloqLibrary JSON parser.
 * @details Parses a Qualtran BloqLibrary JSON string into a protobuf message.
 */

#ifndef QRET_PARSER_QUALTRAN_H
#define QRET_PARSER_QUALTRAN_H

#include <string>

#include "qret/qret_export.h"

#include "qualtran/protos/bloq.pb.h"

namespace qret::qualtran {

/**
 * @brief Parse a Qualtran BloqLibrary JSON string into a protobuf message.
 *
 * @param bloq_library_json  JSON text serialised from a Qualtran BloqLibrary proto.
 * @return Parsed BloqLibrary protobuf message.
 * @throws QRETError(MalformedQualtranProto) if the JSON is structurally invalid.
 */
QRET_EXPORT ::qualtran::BloqLibrary ParseQualtranJson(const std::string& bloq_library_json);

}  // namespace qret::qualtran

#endif  // QRET_PARSER_QUALTRAN_H
