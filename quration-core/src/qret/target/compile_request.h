/**
 * @file qret/target/compile_request.h
 * @brief Common compile request for target backends.
 */

#ifndef QRET_TARGET_COMPILE_REQUEST_H
#define QRET_TARGET_COMPILE_REQUEST_H

#include <string>

#include "qret/target/compile_format.h"

namespace qret {
struct CompileRequest {
    std::string input;
    std::string function_name;
    std::string output;
    std::string target_name;
    CompileFormat source_format = CompileFormat::IR;
};
}  // namespace qret

#endif  // QRET_TARGET_COMPILE_REQUEST_H
