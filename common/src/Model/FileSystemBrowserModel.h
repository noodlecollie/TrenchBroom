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

#include "IO/Path.h"
#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QScopedPointer>
#include <QString>

namespace TrenchBroom {
namespace IO {
class FileSystem;
}

namespace Model {
class FileSystemBrowserModel : public QAbstractItemModel {
  Q_OBJECT
public:
  enum DataRole {
    // Mappings to canonical Qt roles:
    ROLE_PATH = Qt::DisplayRole,

    // Custom roles:
    ROLE_METAFLAGS = Qt::UserRole
  };

  enum MetaFlag {
    METAFLAG_IS_DIRECTORY = (1 << 0)
  };

  FileSystemBrowserModel(IO::FileSystem* fs, QObject* parent = nullptr);

  void reset();

  Qt::ItemFlags flags(const QModelIndex& index) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index) const override;
  bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

private:
  class Node {
  public:
    enum class NodeType {
      UNKNOWN = 0,
      DIRECTORY,
      FILE
    };

    Node()
      : m_fullPath("") {}

    Node(const IO::Path& fullPath, Node* parent = nullptr, int indexInParent = 0)
      : m_parent(parent)
      , m_indexInParent(indexInParent)
      , m_fullPath(fullPath) {}

    ~Node() { destroyChildren(); }

    Node* parent() const { return m_parent; }
    int indexInParent() const { return m_indexInParent; }
    const IO::Path& fullPath() const { return m_fullPath; }
    NodeType nodeType() const { return m_type; }
    bool isRoot() const { return m_parent == nullptr; }
    bool hasChildren() const { return m_children && m_children->count() > 0; }
    bool isEvaluated() const { return m_isEvaluated; }
    void setIsEvaluated(bool evaluated) { m_isEvaluated = evaluated; }

    void setIsDirectory(bool isDir) {
      if ((isDir && m_type == NodeType::DIRECTORY) || (!isDir && m_type == NodeType::FILE)) {
        return;
      }

      destroyChildren();
      m_type = isDir ? NodeType::DIRECTORY : NodeType::FILE;

      if (m_type == NodeType::DIRECTORY) {
        m_children.reset(new QList<Node*>());
      }
    }

    QList<Node*>* childList() const { return m_children.get(); }

    int metaFlags() const {
      int outFlags = 0;

      if (m_type == NodeType::DIRECTORY) {
        outFlags |= METAFLAG_IS_DIRECTORY;
      }

      return outFlags;
    }

  private:
    void destroyChildren() {
      if (m_children) {
        qDeleteAll(*m_children);
        m_children.reset();
      }
    }

    Node* m_parent = nullptr;
    int m_indexInParent = 0;
    IO::Path m_fullPath;
    QScopedPointer<QList<Node*>> m_children;
    bool m_isEvaluated = false;
    NodeType m_type = NodeType::UNKNOWN;
  };

  using NodeHash = QHash<quintptr, Node*>;

  static quintptr nodeToID(const Node* node);

  Node* getNode(quintptr id);
  const Node* getNode(quintptr id) const;

  Node* getNode(const QModelIndex& index);
  const Node* getNode(const QModelIndex& index) const;

  void populateNode(Node& node) const;
  void cheapDetermineIsDirectory(Node& node) const;

  IO::FileSystem* m_fs = nullptr;
  QScopedPointer<Node> m_rootFSNode;
  mutable NodeHash m_nodeHash;
};
} // namespace Model
} // namespace TrenchBroom
