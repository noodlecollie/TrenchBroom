/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "kdl/string_format.h"
#include <sstream>
#include <string>

namespace TrenchBroom {
namespace IO {
namespace ValveKeyValuesWriterUtils {
static inline std::string escapeSpecialWhitespace(const std::string_view str) {
  if (str.empty()) {
    return "";
  }

  std::stringstream buffer;
  for (const auto c : str) {
    switch (c) {
      case '\n': {
        buffer << "\\n";
        break;
      }

      case '\t': {
        buffer << "\\t";
        break;
      }

      case '\r': {
        buffer << "\\r";
        break;
      }

      default: {
        buffer << c;
        break;
      }
    }
  }

  return buffer.str();
}

static inline std::string quoteEscapedString(const std::string_view str) {
  return "\"" + kdl::str_escape(escapeSpecialWhitespace(str), "\"") + "\"";
}
} // namespace ValveKeyValuesWriterUtils
} // namespace IO
} // namespace TrenchBroom
