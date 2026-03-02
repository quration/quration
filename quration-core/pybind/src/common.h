#pragma once

#include <sstream>
#include <string>

namespace pyqret {
template <typename T>
std::string BindFmtUsingOstream(const T& x) {
    auto ss = std::stringstream();
    ss << x;
    return ss.str();
}
}  // namespace pyqret
