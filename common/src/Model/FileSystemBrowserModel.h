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

#include <QAbstractItemModel>

namespace TrenchBroom {
namespace IO {
class FileSystem;
}

namespace Model {
class FileSystemBrowserModel : public QAbstractItemModel {
  Q_OBJECT
public:
  FileSystemBrowserModel(IO::FileSystem* fs, QObject* parent = nullptr);

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
  IO::FileSystem* m_fs = nullptr;
};
} // namespace Model
} // namespace TrenchBroom
