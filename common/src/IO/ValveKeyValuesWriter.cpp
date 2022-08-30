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

#include "IO/ValveKeyValuesWriter.h"
#include "Ensure.h"
#include "IO/ValveKeyValuesTree.h"
#include "IO/ValveKeyValuesWriterUtils.h"
#include "kdl/string_format.h"
#include <fmt/format.h>
#include <iterator> // for std::ostreambuf_iterator

namespace TrenchBroom {
namespace IO {
using namespace ValveKeyValuesWriterUtils;

template <typename... ARGS> static inline void format(std::ostream& out, ARGS&&... args) {
  fmt::format_to(std::ostreambuf_iterator<char>(out), std::forward<ARGS>(args)...);
}

ValveKeyValuesWriter::ValveKeyValuesWriter(std::ostream& stream)
  : m_stream(stream) {}

size_t ValveKeyValuesWriter::countOutputLines(const ValveKeyValuesTree& tree) {
  return countOutputLines(tree.getRoot());
}

size_t ValveKeyValuesWriter::countOutputLines(const ValveKeyValuesNode* node) {
  if (!node) {
    return 0;
  }

  if (node->isRoot()) {
    return computeChildrenOutputLinesDepthFirst(*node, 0);
  } else {
    return computeOutputLinesDepthFirst(*node, 0);
  }
}

void ValveKeyValuesWriter::write(const ValveKeyValuesTree& tree, size_t baseIndent) {
  write(tree.getRoot(), baseIndent);
}

void ValveKeyValuesWriter::write(const ValveKeyValuesNode* node, size_t baseIndent) {
  if (!node) {
    return;
  }

  m_indentString = std::string(baseIndent, '\t');

  if (node->isRoot()) {
    writeNodeChildrenDepthFirst(*node);
  } else {
    writeNodeDepthFirst(*node);
  }
}

size_t ValveKeyValuesWriter::computeOutputLinesDepthFirst(
  const ValveKeyValuesNode& node, size_t currentTotal) {
  // Every node has a key.
  ++currentTotal;

  // If the node is a leaf, the value goes straight after on the same line,
  // and that's it.
  if (node.isLeaf()) {
    return currentTotal;
  }

  // If the node is not a leaf, it has an opening and closing brace,
  // each on their own lines.
  currentTotal += 2;

  // In between are the lines for all children.
  return computeChildrenOutputLinesDepthFirst(node, currentTotal);
}

size_t ValveKeyValuesWriter::computeChildrenOutputLinesDepthFirst(
  const ValveKeyValuesNode& node, size_t currentTotal) {
  // TODO: Iterators would be nice
  for (size_t index = 0; index < node.getChildCount(); ++index) {
    currentTotal = computeOutputLinesDepthFirst(*node.getChild(index), currentTotal);
  }

  return currentTotal;
}

void ValveKeyValuesWriter::writeNodeDepthFirst(const ValveKeyValuesNode& node) {
  format(m_stream, "{}{}", m_indentString, quoteEscapedString(node.getKey()));

  if (node.isLeaf()) {
    format(m_stream, " {}\n", quoteEscapedString(node.getValueString()));
    return;
  }

  format(m_stream, "\n{}{{\n", m_indentString);
  increaseIndent();
  writeNodeChildrenDepthFirst(node);
  decreaseIndent();
  format(m_stream, "{}}}\n", m_indentString);
}

void ValveKeyValuesWriter::writeNodeChildrenDepthFirst(const ValveKeyValuesNode& node) {
  // TODO: Iterators would be nice
  for (size_t index = 0; index < node.getChildCount(); ++index) {
    writeNodeDepthFirst(*node.getChild(index));
  }
}

void ValveKeyValuesWriter::increaseIndent() {
  m_indentString += "\t";
}
void ValveKeyValuesWriter::decreaseIndent() {
  ensure(!m_indentString.empty(), "Unable to decrease empty indent!");
  m_indentString.resize(m_indentString.size() - 1);
}
} // namespace IO
} // namespace TrenchBroom
