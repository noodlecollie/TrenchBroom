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

#include <QSortFilterProxyModel>

namespace TrenchBroom {
namespace Model {
class FileSystemBrowserModel;

class FileSystemFileViewProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
public:
  explicit FileSystemFileViewProxyModel(QObject* parent = nullptr);

  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  void setRootForFiltering(const QModelIndex& sourceIndex);

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
  bool isFilterRootOrDirectAncestor(const QModelIndex& sourceIndex) const;
  bool indexRepresentsFile(const QModelIndex& sourceIndex) const;
  bool pathPassesFilter(const QModelIndex& sourceIndex) const;

private:
  QModelIndex m_rootForFiltering;
};
} // namespace Model
} // namespace TrenchBroom
