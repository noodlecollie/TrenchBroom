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

#include "Model/FileSystemBrowserModel.h"
#include "IO/FileSystem.h"
#include "IO/Path.h"

namespace TrenchBroom {
namespace Model {
FileSystemBrowserModel::FileSystemBrowserModel(IO::FileSystem* fs, QObject* parent)
  : QAbstractItemModel(parent)
  , m_fs(fs)
  , m_rootFSNode(new Node()) {
  populateNode(*m_rootFSNode);
}

void FileSystemBrowserModel::reset() {
  beginResetModel();

  m_nodeHash.clear();
  m_rootFSNode.reset(new Node());
  populateNode(*m_rootFSNode);

  endResetModel();
}

Qt::ItemFlags FileSystemBrowserModel::flags(const QModelIndex& index) const {
  // TODO
  Q_UNUSED(index);
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant FileSystemBrowserModel::data(const QModelIndex& index, int role) const {
  // TODO
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  return QVariant(QString("%0 Row %1, Col %2")
                    .arg(index.parent().isValid() ? "C" : "P")
                    .arg(index.row())
                    .arg(index.column()));
}

QVariant FileSystemBrowserModel::headerData(
  int /* section */, Qt::Orientation orientation, int role) const {
  // TODO
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal) {
      return QVariant("H Header");
    } else {
      return QVariant("V Header");
    }
  }

  return QVariant();
}

int FileSystemBrowserModel::rowCount(const QModelIndex& parent) const {
  // Starting out with just one item underneath the root.
  // TODO: Complete this properly.
  return (!parent.isValid()) ? 1 : 0;
}

int FileSystemBrowserModel::columnCount(const QModelIndex& parent) const {
  // Only one column for now.
  return (!parent.isValid()) ? 1 : 0;
}

QModelIndex FileSystemBrowserModel::index(int row, int column, const QModelIndex& parent) const {
  // For now, only deal with the root.
  if (parent.isValid()) {
    return QModelIndex();
  }

  // TODO: Support more than one row.
  if (row != 0 || column != 0) {
    return QModelIndex();
  }

  // TODO
}

QModelIndex FileSystemBrowserModel::parent(const QModelIndex& index) const {
  // TODO
  const quintptr parentData = index.internalId();
  return parentData != 0
           ? createIndex(static_cast<int>(parentData - 1), 0, static_cast<quintptr>(0))
           : QModelIndex();
}

bool FileSystemBrowserModel::hasChildren(const QModelIndex& parent) const {
  const Node* node = getNode(parent);
  return node ? node->hasChildren() : false;
}

FileSystemBrowserModel::Node* FileSystemBrowserModel::getNode(quintptr id) {
  NodeHash::iterator it = m_nodeHash.find(id);
  return it != m_nodeHash.end() ? it->second : nullptr;
}

const FileSystemBrowserModel::Node* FileSystemBrowserModel::getNode(quintptr id) const {
  NodeHash::const_iterator it = m_nodeHash.find(id);
  return it != m_nodeHash.cend() ? it->second : nullptr;
}

FileSystemBrowserModel::Node* FileSystemBrowserModel::getNode(const QModelIndex& index) {
  return index.isValid() ? getNode(index.internalId()) : m_rootFSNode.get();
}

const FileSystemBrowserModel::Node* FileSystemBrowserModel::getNode(
  const QModelIndex& index) const {
  return index.isValid() ? getNode(index.internalId()) : m_rootFSNode.get();
}

void FileSystemBrowserModel::populateNode(Node& /* node */) {
  // TODO: Look up the directory in the filesystem and add children for this node.
}

quintptr FileSystemBrowserModel::nodeToID(Node* node) {
  return reinterpret_cast<quintptr>(node);
}
} // namespace Model
} // namespace TrenchBroom
