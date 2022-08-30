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

#include "IO/ValveKeyValuesTree.h"

namespace TrenchBroom {
namespace IO {
ValveKeyValuesNode::ValveKeyValuesNode(const std::string& key, ValveKeyValuesNode* parent)
  : m_key(key)
  , m_parent(parent) {}

ValveKeyValuesNode* ValveKeyValuesNode::getParent() const {
  return m_parent;
}

bool ValveKeyValuesNode::isRoot() const {
  return m_parent == nullptr;
}

std::string ValveKeyValuesNode::getKey() const {
  return m_key;
}

void ValveKeyValuesNode::setKey(const std::string& key) {
  if (isRoot() || key.empty()) {
    return;
  }

  m_key = key;
}

bool ValveKeyValuesNode::isLeaf() const {
  return !m_value.empty();
}

ValveKeyValuesNode* ValveKeyValuesNode::addChild(const std::string& key) {
  if (key.empty()) {
    return nullptr;
  }

  // No longer a leaf node.
  m_value.clear();

  m_children.emplace_back(NodeUniquePtr(new ValveKeyValuesNode(key, this)));
  return m_children.back().get();
}

ValveKeyValuesNode* ValveKeyValuesNode::addChildWithValue(
  const std::string& key, const std::string& value) {
  ValveKeyValuesNode* child = addChild(key);

  if (child) {
    child->setValueString(value);
  }

  return child;
}

bool ValveKeyValuesNode::removeChild(ValveKeyValuesNode* child) {
  if (child) {
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
      if (it->get() == child) {
        m_children.erase(it);
        return true;
      }
    }
  }

  return false;
}

size_t ValveKeyValuesNode::getChildCount() const {
  return m_children.size();
}

ValveKeyValuesNode* ValveKeyValuesNode::getChild(size_t index) {
  return index < m_children.size() ? m_children[index].get() : nullptr;
}

const ValveKeyValuesNode* ValveKeyValuesNode::getChild(size_t index) const {
  return index < m_children.size() ? m_children[index].get() : nullptr;
}

void ValveKeyValuesNode::clearChildren() {
  m_children.clear();
}

std::string ValveKeyValuesNode::getValueString() const {
  return m_value;
}

void ValveKeyValuesNode::setValueString(const std::string& value) {
  if (isRoot()) {
    return;
  }

  // No longer a non-leaf node.
  m_children.clear();

  m_value = value;
}

ValveKeyValuesTree::ValveKeyValuesTree()
  : m_root(new ValveKeyValuesNode()) {}

ValveKeyValuesTree::ValveKeyValuesTree(ValveKeyValuesTree&& other)
  : m_root(std::move(other.m_root)) {
  // Trees should always have a root node, so reset the other tree's root now that we've stolen
  // it.
  other.m_root.reset(new ValveKeyValuesNode());
}

ValveKeyValuesTree& ValveKeyValuesTree::operator=(ValveKeyValuesTree&& other) {
  m_root = std::move(other.m_root);

  // Trees should always have a root node, so reset the other tree's root now that we've stolen
  // it.
  other.m_root.reset(new ValveKeyValuesNode());

  return *this;
}

ValveKeyValuesNode* ValveKeyValuesTree::getRoot() const {
  return m_root.get();
}
} // namespace IO
} // namespace TrenchBroom
