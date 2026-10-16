/**
 * @file qret/parser/qualtran.cpp
 * @brief Qualtran BloqLibrary JSON parser.
 */

#include "qret/parser/qualtran.h"

#include <fmt/format.h>

#include <google/protobuf/util/json_util.h>
#include <string>

#include "qret/error.h"
#include "qret/exception.h"

namespace qret::qualtran {

::qualtran::BloqLibrary ParseQualtranJson(const std::string& bloq_library_json) {
    auto lib = ::qualtran::BloqLibrary{};
    const auto status = google::protobuf::util::JsonStringToMessage(bloq_library_json, &lib);
    if (!status.ok()) {
        throw QRETError(
                error::MalformedQualtranProto,
                fmt::format("failed to parse BloqLibrary JSON: {}", std::string(status.message()))
        );
    }
    if (lib.table_size() == 0) {
        throw QRETError(error::MalformedQualtranProto, "BloqLibrary table is empty");
    }
    return lib;
}

}  // namespace qret::qualtran
