/**
 * @file qret/base/gridsynth.h
 * @brief Wrapper of GridSynth.
 * @details This file provides a portable wrapper of GridSynth.
 * Reference of GridSynth: https://www.mathstat.dal.ca/~selinger/newsynth/
 */

#ifndef QRET_BASE_GRIDSYNTH_H
#define QRET_BASE_GRIDSYNTH_H

#include <cstdint>
#include <string>
#include <variant>

#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Get the GridSynth path.
 * @details Search the path in the following order.
 *
 * 1. Is the environment variable GRIDSYNTH_PATH is defined ?
 * 2. Does the binary `gridsynth` exist in the path installed by default in cabal?
 * 3. Does the binary `gridsynth` exist in the working directory?
 * 4. If the above conditions are not met, returns `gridsynth` or `gridsynth.exe`.
 *
 * @return std::string path
 */
QRET_EXPORT std::string GetGridSynthPath() noexcept;

/**
 * @brief Check if the binary in the path is executable.
 *
 * @param path path of GridSynth
 * @return true runnable
 * @return false not runnable
 */
QRET_EXPORT bool IsGridSynthRunnable(const std::string& path) noexcept;

/**
 * @brief Check if the GridSynth is runnable.
 * @details If you want to determine whether the binary path is not correctly obtained or the binary
 * itself is inappropriate, use functions such as GetGridSynthPath() and IsGridSynthRunnable(const
 * std:string& path).
 *
 * @return true runnable
 * @return false not runnable
 */
inline bool IsGridSynthRunnable() noexcept {
    return IsGridSynthRunnable(GetGridSynthPath());
}

/**
 * @brief Approximate z-rotation with Clifford+T gate set.
 * @details z-rotation matrix:
 *
 * \f[
 *   R_z(\theta) =
 *   \begin{bmatrix}
 *     e^{-i\theta/2} & 0 \\
 *     0               & e^{i\theta/2}
 *   \end{bmatrix}
 * \f]
 *
 *
 *@warning The definition of z-rotation in GridSynth is different from qret::ir:
 * \f$R_z(\theta)\f$ in GridSynth = \f$R_z(- \theta)\f$ in IR
 *
 * @param theta z-rotation angle
 * @param out string of Clifford+T gate set
 * @param exit exit status of the GridSynth command
 * @param precision precision (default: 10^-10)
 * * If `std::uint32_t`: set precision in decimal digits
 * * If `double`: set precision as epsilon
 * @param phase decompose up to a global phase (default: no)
 * @param effort how hard to try to factor (default: 25)
 * @param rseed set random seed. when rseed is 0, seed is random (default: 10)
 */
QRET_EXPORT void ApproximateRzWithCliffordT(
        double theta,
        std::string& out,
        std::int32_t& exit,
        std::variant<std::uint32_t, double> precision = 10u,
        bool phase = false,
        std::uint32_t effort = 25,
        std::uint32_t rseed = 10
);
}  // namespace qret

#endif  // QRET_BASE_GRIDSYNTH_H
