/**
 * @file qret/base/system.cpp
 * @brief Wrapper of OS-dependent functionality.
 */

#include "qret/base/system.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

namespace qret {
void RunCommand(const std::string& cmd, std::string& out, std::int32_t& exit) {
    constexpr auto BufSize = std::size_t{256};

    // Run the command
    const auto pipe = std::shared_ptr<FILE>(
            POPEN(cmd.c_str(), "r"),  // NOLINT
            [&](FILE* p) { exit = PCLOSE(p); }
    );
    if (!pipe) {
        throw std::runtime_error("failed to run command: " + cmd);
    }
    if (exit != 0) {
        return;
    }

    // Copy stdout from buffer
    auto buf = std::array<char, BufSize>();
    while (feof(pipe.get()) == 0) {
        if (fgets(buf.data(), buf.size(), pipe.get()) != nullptr) {
            out += buf.data();
        }
    }

    // Remove '\n' at the end
    while (!out.empty() && out.back() == '\n') {
        out.pop_back();
    }
}
}  // namespace qret
