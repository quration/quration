/**
 * @file qret/base/option.cpp
 * @brief Easy way to add command line option.
 */

#include "qret/base/option.h"

namespace qret {
OptionStorage* OptionStorage::GetOptionStorage() {
    static OptionStorage option_storage;
    return &option_storage;
}
}  // namespace qret
