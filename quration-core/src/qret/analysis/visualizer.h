/**
 * @file qret/analysis/visualizer.h
 * @brief Visualize a func in various ways.
 */

#ifndef QRET_ANALYSIS_VISUALIZE_H
#define QRET_ANALYSIS_VISUALIZE_H

#include <string>

#include "qret/ir/basic_block.h"
#include "qret/ir/function.h"
#include "qret/ir/instruction.h"
#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Get the DOT string representing Control Flow Graph.
 */
QRET_EXPORT std::string GenCFG(const ir::Function& func);

/**
 * @brief Get the DOT string representing Call Graph.
 *
 * @param func func to draw.
 * @param display_num_calls display how many times it is called
 */
QRET_EXPORT std::string GenCallGraph(const ir::Function& func, bool display_num_calls = false);

/**
 * @brief Generate LaTeX source code that represents a func diagram.
 * @note Generated code requires "quantikz" package.
 * @details For some instructions, such as Call, ClassicalFunction, and CRand, the width of the box
 * is automatically expanded according to the length of the name. The box may not fit depending on
 * the content of the text. You can resize boxes by manually editing LaTeX source.
 *
 * @return std::string LaTeX source of the func diagram
 */
QRET_EXPORT std::string GenLaTeX(const ir::Function& func);

/**
 * @brief Get the DOT string representing Compute Graph.
 */
QRET_EXPORT std::string GenComputeGraph(const ir::Function& func);
}  // namespace qret

#endif  // QRET_ANALYSIS_VISUALIZE_H
