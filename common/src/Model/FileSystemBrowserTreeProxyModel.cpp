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

#include "Model/FileSystemBrowserTreeProxyModel.h"
#include "Model/FileSystemBrowserModel.h"

namespace TrenchBroom {
namespace Model {
FileSystemBrowserTreeProxyModel::FileSystemBrowserTreeProxyModel(QObject* parent)
  : QSortFilterProxyModel(parent) {}

bool FileSystemBrowserTreeProxyModel::filterAcceptsRow(
  int sourceRow, const QModelIndex& sourceParent) const {
  QAbstractItemModel* src = sourceModel();

  if (!src) {
    return false;
  }

  const QModelIndex srcIndex = src->index(sourceRow, 0, sourceParent);
  const QVariant flagVar = src->data(srcIndex, Model::FileSystemBrowserModel::ROLE_METAFLAGS);

  return flagVar.isValid() &&
         (flagVar.toInt() & Model::FileSystemBrowserModel::METAFLAG_IS_DIRECTORY);
}
} // namespace Model
} // namespace TrenchBroom
