/**
 * @file qret/codegen/dummy.cpp
 * @brief Dummy pass.
 */

#include "qret/codegen/dummy.h"

namespace qret {
namespace {
static auto X = RegisterPass<DummyPass>("DummyPass", "dummy");
}
}  // namespace qret
