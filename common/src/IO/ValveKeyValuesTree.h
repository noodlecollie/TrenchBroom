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

#include "Macros.h"
#include <memory>
#include <string>
#include <vector>

namespace TrenchBroom {
namespace IO {
class ValveKeyValuesNode {
public:
  bool isRoot() const;

  // Returns nullptr if this node is a root node.
  ValveKeyValuesNode* getParent() const;

  std::string getKey() const;
  void setKey(const std::string& key);

  // Returns whether this node's value is empty.
  // A leaf node may contain a value; a non-leaf node may not.
  // Setting a value on a non-leaf node will destroy all its children,
  // and adding a child to a leaf node will clear its existing value.
  bool isLeaf() const;

  // Once this node dies, all added children die as well.
  // Any existing value in this node is erased when a child is added.
  ValveKeyValuesNode* addChild(const std::string& key);
  ValveKeyValuesNode* addChildWithValue(const std::string& key, const std::string& value);

  // Returns true if the child was successfully removed from this node.
  // In this case, the child pointer is no longer valid.
  bool removeChild(ValveKeyValuesNode* child);

  size_t getChildCount() const;
  ValveKeyValuesNode* getChild(size_t index);
  const ValveKeyValuesNode* getChild(size_t index) const;
  void clearChildren();

  std::string getValueString() const;

  // Any existing children are killed.
  void setValueString(const std::string& value);

private:
  friend class ValveKeyValuesTree;

  using NodeUniquePtr = std::unique_ptr<ValveKeyValuesNode>;

  explicit ValveKeyValuesNode(
    const std::string& key = std::string(), ValveKeyValuesNode* parent = nullptr);

  deleteCopyAndMove(ValveKeyValuesNode);

  // These do not make sense in the context of a tree node.
  // Pointer comparison is enough for a quick equality compare.
  bool operator==(const ValveKeyValuesNode& other) const = delete;
  bool operator!=(const ValveKeyValuesNode& other) const = delete;

  std::string m_key;
  ValveKeyValuesNode* m_parent = nullptr;
  std::vector<NodeUniquePtr> m_children;
  std::string m_value;
};

class ValveKeyValuesTree {
public:
  ValveKeyValuesTree();

  // TODO: Make these perform a deep copy
  ValveKeyValuesTree(const ValveKeyValuesTree& other) = delete;
  ValveKeyValuesTree& operator=(const ValveKeyValuesTree& other) = delete;

  ValveKeyValuesTree(ValveKeyValuesTree&& other);
  ValveKeyValuesTree& operator=(ValveKeyValuesTree&& other);

  // Root is owned by the tree. Once the tree dies, all descendant nodes die as well.
  ValveKeyValuesNode* getRoot() const;

private:
  // Note that we can't use std::make_unique() to initialise this,
  // as this doesn't seem to work for friend classes. We have to
  // use normal new instead.
  std::unique_ptr<ValveKeyValuesNode> m_root;
};
} // namespace IO
} // namespace TrenchBroom
