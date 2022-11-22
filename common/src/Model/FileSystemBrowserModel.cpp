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
#include <qabstractitemmodel.h>
#include <qnamespace.h>
#include <qvariant.h>

namespace TrenchBroom {
namespace Model {
Qt::ItemFlags FileSystemBrowserModel::flags(const QModelIndex& index) const {
  // TODO
  Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;

  if (index.parent().isValid()) {
    flags |= Qt::ItemNeverHasChildren;
  }

  return flags;
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
  // TODO
  return parent.isValid() ? 1 : 5;
}

int FileSystemBrowserModel::columnCount(const QModelIndex& /* parent */) const {
  // TODO
  return 2;
}

QModelIndex FileSystemBrowserModel::index(int row, int column, const QModelIndex& parent) const {
  // TODO
  if (parent.isValid() && row != 0) {
    return QModelIndex();
  }

  return createIndex(row, column, static_cast<quintptr>(parent.isValid() ? parent.row() + 1 : 0));
}

QModelIndex FileSystemBrowserModel::parent(const QModelIndex& index) const {
  // TODO
  const quintptr parentData = index.internalId();
  return parentData != 0
           ? createIndex(static_cast<int>(parentData - 1), 0, static_cast<quintptr>(0))
           : QModelIndex();
}
} // namespace Model
} // namespace TrenchBroom
