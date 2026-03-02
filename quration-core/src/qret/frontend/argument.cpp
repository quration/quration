/**
 * @file qret/frontend/argument.cpp
 * @brief Arguments of quantum circuit.
 */

#include "qret/frontend/argument.h"

#include <utility>

#include "qret/frontend/builder.h"  // DO NOT DELETE
#include "qret/frontend/circuit.h"  // DO NOT DELETE
#include "qret/frontend/circuit_generator.h"  // DO NOT DELETE

namespace qret::frontend {
std::ostream& operator<<(std::ostream& out, const Qubit& q) {
    return out << 'q' << q.id_;
}
std::ostream& operator<<(std::ostream& out, const Register& r) {
    return out << 'r' << r.id_;
}
std::ostream& operator<<(std::ostream& out, const Qubits& qs) {
    const char* sep = "";
    out << '{';
    for (const auto& q : qs) {
        out << std::exchange(sep, ",") << q;
    }
    return out << '}';
}
std::ostream& operator<<(std::ostream& out, const Registers& rs) {
    const char* sep = "";
    out << '{';
    for (const auto& r : rs) {
        out << std::exchange(sep, ",") << r;
    }
    return out << '}';
}
template class QuantumBase<ObjectType::Qubit, SizeType::Single>;  // Qubit
template class QuantumBase<ObjectType::Qubit, SizeType::Array>;  // Qubits
template class QuantumBase<ObjectType::Register, SizeType::Single>;  // Register
template class QuantumBase<ObjectType::Register, SizeType::Array>;  // Registers
}  // namespace qret::frontend
