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

#include <ostream>

namespace TrenchBroom {
namespace IO {
class ValveKeyValuesTree;
class ValveKeyValuesNode;

class ValveKeyValuesWriter {
public:
  ValveKeyValuesWriter(std::ostream& stream);

  static size_t countOutputLines(const ValveKeyValuesTree& tree);
  static size_t countOutputLines(const ValveKeyValuesNode* node);

  void write(const ValveKeyValuesTree& tree, size_t baseIndent = 0);
  void write(const ValveKeyValuesNode* node, size_t baseIndent = 0);

private:
  static size_t computeOutputLinesDepthFirst(const ValveKeyValuesNode& node, size_t currentTotal);
  static size_t computeChildrenOutputLinesDepthFirst(
    const ValveKeyValuesNode& node, size_t currentTotal);

  void writeNodeDepthFirst(const ValveKeyValuesNode& node);
  void writeNodeChildrenDepthFirst(const ValveKeyValuesNode& node);
  void increaseIndent();
  void decreaseIndent();

  std::ostream& m_stream;
  std::string m_indentString;
};
} // namespace IO
} // namespace TrenchBroom
