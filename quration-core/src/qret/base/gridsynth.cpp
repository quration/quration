/**
 * @file qret/base/gridsynth.h
 * @brief Wrapper of GridSynth.
 */

#include "qret/base/gridsynth.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>  // NOLINT
#include <iomanip>
#include <iostream>
#include <mutex>  // NOLINT
#include <sstream>
#include <stdexcept>
#include <string>

#include "qret/base/system.h"

#ifdef _WIN32
#define GRIDSYNTH_BIN "gridsynth.exe"
#else
#define GRIDSYNTH_BIN "gridsynth"
#endif

namespace qret {
std::string GetGridSynthPath() noexcept {
    // 1. Is the environment variable GRIDSYNTH_PATH is defined ?
    const auto* const path = std::getenv("GRIDSYNTH_PATH");
    if (path != nullptr) {
        return path;
    }

    auto gridsynth_path = std::string(GRIDSYNTH_BIN);
    auto find_gridsynth = [&gridsynth_path](const std::string& dir) {
        if (!std::filesystem::exists(dir)) {
            return false;
        }
        auto itr = std::filesystem::directory_iterator(dir);
        const auto found = std::find_if(itr, std::filesystem::end(itr), [](const auto& dir_entry) {
            return dir_entry.path().filename() == GRIDSYNTH_BIN;
        });
        if (found != std::filesystem::end(itr)) {
            gridsynth_path = found->path().string();
            return true;
        }
        return false;
    };

    // 2. Does the binary `gridsynth` exist in the path installed by default in cabal?
#ifdef _WIN32
    if (find_gridsynth(R"(C:\cabal\bin)")) {
        return gridsynth_path;
    }
#else
    const auto* const home_ptr = std::getenv("HOME");
    const auto home = home_ptr == nullptr ? std::string("/home") : std::string(home_ptr);
    if (find_gridsynth(home + "/.cabal/bin")) {
        return gridsynth_path;
    }
#endif

    // 3. Does the binary `gridsynth` exist in the working directory?
    const auto working_dir = std::filesystem::current_path().string();
    if (find_gridsynth(working_dir)) {
        return gridsynth_path;
    }

    // 4. If the above conditions are not met, returns `gridsynth` or `gridsynth.exe`.
    return gridsynth_path;
}

bool IsGridSynthRunnable(const std::string& path) noexcept {
    static std::once_flag once;
    auto print_error =
            [](const std::string& cmd, const std::string& expected, const std::string& actual) {
                std::cerr << "Unexpected output of command: " << cmd << std::endl;
                std::cerr << "  Expected: " << expected << std::endl;
                std::cerr << "  Actual  : " << actual << std::endl;
            };
    auto check_command = [print_error](const std::string& cmd, const std::string& expected) {
        auto out = std::string();
        auto exit = std::int32_t();
        try {
            RunCommand(cmd, out, exit);
        } catch (std::runtime_error& e) {
            return false;
        }
        if (exit != 0) {
            return false;
        }
        if (expected != out) {
            std::call_once(once, print_error, cmd, expected, out);
        }
        return true;
    };
    constexpr auto GridSynthHelp =
            R"(Usage: gridsynth [OPTION...] <theta>
Arguments:
 <theta>                   z-rotation angle. May be symbolic, e.g. pi/128
Options:
  -h        --help          print usage info and exit
  -d <n>    --digits=<n>    set precision in decimal digits (default: 10)
  -b <n>    --bits=<n>      set precision in bits
  -e <n>    --epsilon=<n>   set precision as epsilon (default: 1e-10)
  -p        --phase         decompose up to a global phase (default: no)
  -f "<n>"  --effort="<n>"  how hard to try to factor (default: 25)
  -x        --hex           output hexadecimal coding (default: ASCII)
  -s        --stats         output statistics
  -l        --latex         use LaTeX output format
  -t        --table         generate the table of results for the article
  -c <n>    --count=<n>     repeat count for --table mode (default: 50)
  -r "<s>"  --rseed="<s>"   set optional random seed (default: random))";
    return check_command(path + " --help", GridSynthHelp) && check_command(path + " 0", "I")
            && check_command(path + " pi -p", "SS") && check_command(path + " pi/2 -p", "S")
            && check_command(path + " pi/4 -p", "T");
}

void ApproximateRzWithCliffordT(
        double theta,
        std::string& out,
        std::int32_t& exit,
        std::variant<std::uint32_t, double> precision,
        bool phase,
        std::uint32_t effort,
        std::uint32_t rseed
) {
    const auto gridsynth_path = GetGridSynthPath();
    if (!IsGridSynthRunnable(gridsynth_path)) {
        throw std::runtime_error(
                "cannot run gridsynth for this path: " + gridsynth_path
                + "\nPlease set the environment variable GRIDSYNTH_PATH correctly."
        );
    }
    auto cmd = std::stringstream();
    cmd << gridsynth_path + " ";
    cmd << std::fixed << std::setprecision(30) << "\"(" << theta << ")\"";
    if (precision.index() == 0) {
        const auto eps_digits = std::get<0>(precision);
        cmd << " -d " << eps_digits;
    } else if (precision.index() == 1) {
        const auto epsilon = std::get<1>(precision);
        cmd << " -e " << epsilon;
    }
    if (phase) {
        cmd << " -p";
    }
    if (effort != 25) {
        cmd << " -f " << effort;
    }
    if (rseed != 0) {
        cmd << " -r " << rseed;
    }
    RunCommand(cmd.str(), out, exit);
}
}  // namespace qret
