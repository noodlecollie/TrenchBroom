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

#include "Model/FileSystemBrowserTableProxyModel.h"
#include "Model/FileSystemBrowserModel.h"
#include <QtDebug>

namespace TrenchBroom {
namespace Model {
FileSystemBrowserTableProxyModel::FileSystemBrowserTableProxyModel(QObject* parent)
  : QSortFilterProxyModel(parent) {}

QVariant FileSystemBrowserTableProxyModel::headerData(
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

  return QVariant(tr("Files"));
}

void FileSystemBrowserTableProxyModel::setRootForFiltering(const QModelIndex& sourceIndex) {
  if (m_rootForFiltering == sourceIndex) {
    return;
  }

  m_rootForFiltering = sourceIndex;

  // This must be called, otherwise filtering does not get re-run! Re-filtering is required when
  // this index changes, since a directory passes the filter when it is the root, but does not pass
  // otherwise. If a previously encountered directory becomes the new root, it must be re-evaluated
  // by the filter, otherwise none of its children are shown.
  invalidateFilter();
}

bool FileSystemBrowserTableProxyModel::filterAcceptsRow(
  int sourceRow, const QModelIndex& sourceParent) const {
  QAbstractItemModel* src = sourceModel();

  if (!src) {
    return false;
  }

  const QModelIndex srcIndex = src->index(sourceRow, 0, sourceParent);

  if (srcIndex == m_rootForFiltering) {
    // Always allowed, or children don't show up.
    return true;
  }

  const QVariant flagVar = src->data(srcIndex, Model::FileSystemBrowserModel::ROLE_METAFLAGS);

  return flagVar.isValid() &&
         !(flagVar.toInt() & Model::FileSystemBrowserModel::METAFLAG_IS_DIRECTORY);
}
} // namespace Model
} // namespace TrenchBroom
