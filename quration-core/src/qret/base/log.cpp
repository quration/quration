/**
 * @file qret/base/log.cpp
 * @brief Logger.
 */

#include "qret/base/log.h"

namespace qret {
Logger* Logger::GetLogger() {
    static Logger logger;
    return &logger;
}
}  // namespace qret
