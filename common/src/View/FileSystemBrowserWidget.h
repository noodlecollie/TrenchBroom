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

#include <QWidget>
#include <memory>

class QLineEdit;
class QTreeView;
class QTableView;

namespace TrenchBroom {
namespace Model {
class Game;
class FileSystemBrowserModel;
class FileSystemBrowserTreeProxyModel;
class FileSystemBrowserTableProxyModel;
}

namespace View {
class FileSystemBrowserWidget : public QWidget {
  Q_OBJECT
public:
  explicit FileSystemBrowserWidget(
    QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  void setGame(const std::shared_ptr<Model::Game>& game);

private slots:
  void onDirectoryActivated(const QModelIndex& index);
  void onFileActivated(const QModelIndex& index);

private:
  void refresh();
  QString getPathForTableViewItem(const QModelIndex& index) const;

  std::shared_ptr<Model::Game> m_Game;

  QLineEdit* m_filePathTextBox = nullptr;
  QTreeView* m_fileSystemTreeView = nullptr;
  QTableView* m_fileSystemTableView = nullptr;

  Model::FileSystemBrowserModel* m_fsModel = nullptr;
  Model::FileSystemBrowserTreeProxyModel* m_treeProxyModel = nullptr;
  Model::FileSystemBrowserTableProxyModel* m_tableProxyModel = nullptr;
};
} // namespace View
} // namespace TrenchBroom
