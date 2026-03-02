#include "qret/target/sc_ls_fixed_v0/state.h"

#include <gtest/gtest.h>

using namespace qret;
using namespace qret::sc_ls_fixed_v0;

TEST(MagicFactoryState, SizeIs48Byte) {
    static_assert(sizeof(pcg64) == 32);
    static_assert(sizeof(MagicFactoryState) == 48);
}
TEST(QuantumStateBuffer, SizeOfNodeIs16Byte) {
    static_assert(sizeof(QuantumStateBuffer::Node) == 16);
}
