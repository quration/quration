/**
 * @file qret/base/string.h
 * @brief Utility functions for strings.
 * @details This file provides utility functions for working with strings.
 */

#ifndef QRET_BASE_STRING_H
#define QRET_BASE_STRING_H

#include <string>
#include <string_view>
#include <vector>

#include "qret/qret_export.h"

namespace qret {
/**
 * @brief Convert uppercase letters to lowercase.
 * @details Convert 'A' to 'a', 'B' to 'b', ..., 'Z' to 'z'
 *
 * @param sv string
 * @return std::string lowercase string
 */
QRET_EXPORT std::string GetLowerString(std::string_view sv);
/**
 * @brief Split string by separator.
 * @details Examples:
 *
 * * SplitString("", ',') -> {""}
 * * SplitString("^^^", '^') -> {"", "", "", ""}
 * * SplitString("abc,def,ghi", ',') -> {"abc", "def", "ghi"}
 * * SplitString("abc|def||ghi", '|') -> {"abc", "def", "", "ghi"}
 * * SplitString("//abc/def/ghi//", '/') -> {"", "", "abc", "def", "ghi", "", ""}
 *
 * @param sv string
 * @param separator separator
 * @return std::vector<std::string> splitted string
 */
QRET_EXPORT std::vector<std::string> SplitString(std::string_view sv, char separator);
/**
 * @brief Read whole file into std::string.
 *
 * @param filepath filepath
 * @return std::string the content of file
 */
QRET_EXPORT std::string LoadFile(const std::string& filepath);
}  // namespace qret

#endif  // QRET_BASE_STRING_H
