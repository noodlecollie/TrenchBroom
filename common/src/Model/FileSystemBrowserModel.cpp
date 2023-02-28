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
#include <QtDebug>
#include <limits>

namespace TrenchBroom {
namespace Model {
FileSystemBrowserModel::FileSystemBrowserModel(IO::FileSystem* fs, QObject* parent)
  : QAbstractItemModel(parent)
  , m_fs(fs)
  , m_rootFSNode(new Node()) {
  m_nodeHash[nodeToID(m_rootFSNode.get())] = m_rootFSNode.get();
  populateNode(*m_rootFSNode);
}

void FileSystemBrowserModel::reset() {
  beginResetModel();

  m_nodeHash.clear();

  m_rootFSNode.reset(new Node());
  m_nodeHash[nodeToID(m_rootFSNode.get())] = m_rootFSNode.get();
  populateNode(*m_rootFSNode);

  endResetModel();
}

Qt::ItemFlags FileSystemBrowserModel::flags(const QModelIndex& index) const {
  Q_UNUSED(index);
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant FileSystemBrowserModel::data(const QModelIndex& index, int role) const {
  const Node* node = getNode(index);

  if (!node) {
    return QVariant();
  }

  switch (role) {
    case ROLE_PATH: {
      const IO::Path& path = node->fullPath();
      const QString pathString =
        QString::fromStdString((!path.components().empty()) ? path.lastComponent().asString() : "");

      return QVariant((!pathString.isEmpty()) ? pathString : QString("File System"));
    }

    case ROLE_METAFLAGS: {
      return QVariant(node->metaFlags());
    }

    case ROLE_FULL_PATH: {
      const IO::Path& path = node->fullPath();
      return QVariant(QString::fromStdString(path.asString("/")));
    }

    default: {
      return QVariant();
    }
  }
}

QVariant FileSystemBrowserModel::headerData(
  int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (orientation != Qt::Horizontal) {
    return QVariant();
  }

  if (section != 0) {
    return QVariant();
  }

  return QVariant(tr("Node"));
}

int FileSystemBrowserModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    // Just root as a child.
    return 1;
  }

  const Node* node = getNode(parent);
  return node ? node->childList()->count() : 0;
}

int FileSystemBrowserModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    // Root doesn't correspond to a node, but does have columns.
    return 1;
  }

  const Node* node = getNode(parent);
  return node ? 1 : 0;
}

QModelIndex FileSystemBrowserModel::index(int row, int column, const QModelIndex& parent) const {
  if (column < 0 || column >= 1 || row < 0) {
    return QModelIndex();
  }

  if (!parent.isValid()) {
    // This is the index for the root.
    return row == 0 ? createIndex(row, column, nodeToID(m_rootFSNode.get())) : QModelIndex();
  }

  const Node* parentNode = getNode(parent);

  if (!parentNode) {
    return QModelIndex();
  }

  QList<Node*>* children = parentNode->childList();

  if (!children || row >= children->count()) {
    return QModelIndex();
  }

  Node* child = children->at(row);

  if (!child) {
    return QModelIndex();
  }

  populateNode(*child);

  return createIndex(row, column, nodeToID(child));
}

QModelIndex FileSystemBrowserModel::parent(const QModelIndex& index) const {
  if (!index.isValid()) {
    return QModelIndex();
  }

  const Node* node = getNode(index);

  if (!node) {
    return QModelIndex();
  }

  const Node* parent = node->parent();

  if (!parent) {
    return QModelIndex();
  }

  const QModelIndex outIndex =
    createIndex(parent->indexInParent(), 0, nodeToID(parent));
  return outIndex;
}

bool FileSystemBrowserModel::hasChildren(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    // A single child at the root.
    return true;
  }

  const Node* node = getNode(parent);
  return node && node->nodeType() == Node::NodeType::DIRECTORY;
}

FileSystemBrowserModel::Node* FileSystemBrowserModel::getNode(quintptr id) {
  NodeHash::iterator it = m_nodeHash.find(id);
  return it != m_nodeHash.end() ? it.value() : nullptr;
}

const FileSystemBrowserModel::Node* FileSystemBrowserModel::getNode(quintptr id) const {
  NodeHash::const_iterator it = m_nodeHash.find(id);
  return it != m_nodeHash.cend() ? it.value() : nullptr;
}

FileSystemBrowserModel::Node* FileSystemBrowserModel::getNode(const QModelIndex& index) {
  return index.isValid() ? getNode(index.internalId()) : nullptr;
}

const FileSystemBrowserModel::Node* FileSystemBrowserModel::getNode(
  const QModelIndex& index) const {
  return index.isValid() ? getNode(index.internalId()) : nullptr;
}

void FileSystemBrowserModel::populateNode(Node& node) const {
  if (node.isEvaluated()) {
    return;
  }

  try {
    cheapDetermineIsDirectory(node);

    if (node.nodeType() == Node::NodeType::DIRECTORY) {
      // This used to be done by calling getDirectoryContents(),
      // but I don't think this works properly - it throws an
      // exception from the first file system in the chain when
      // the path isn't found there, and doesn't go on to search
      // subsequent file systems in the chain. Not sure what the
      // difference is between getDirectoryContents() and findItems(),
      // but findItems() *does* seem to work.
      const std::vector<IO::Path> contents = m_fs->findItems(node.fullPath());

      QList<Node*>& children = *node.childList();

      for (size_t index = 0;
           index < contents.size() && index < static_cast<size_t>(std::numeric_limits<int>::max());
           ++index) {
        const IO::Path& item = contents[index];
        Node* child = new Node(item, &node, static_cast<int>(index));
        children.append(child);
        m_nodeHash[nodeToID(child)] = child;

        cheapDetermineIsDirectory(*child);
      }
    }
  } catch (const FileSystemException& ex) {
    qWarning() << "Error processing children for node"
               << QString::fromStdString(node.fullPath().asString()) << ":" << QString(ex.what());
  }

  node.setIsEvaluated(true);
}

void FileSystemBrowserModel::cheapDetermineIsDirectory(Node& node) const {
  if (node.nodeType() != Node::NodeType::UNKNOWN) {
    return;
  }

  try {
    node.setIsDirectory(m_fs->directoryExists(node.fullPath()));
  } catch (const FileSystemException&) {
    node.setIsDirectory(false);
  }
}

quintptr FileSystemBrowserModel::nodeToID(const Node* node) {
  return reinterpret_cast<quintptr>(node);
}
} // namespace Model
} // namespace TrenchBroom
