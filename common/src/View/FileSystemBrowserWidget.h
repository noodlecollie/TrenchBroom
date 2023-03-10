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
class QComboBox;
class QSplitter;
class QItemSelection;

namespace TrenchBroom {
namespace Model {
class Game;
class FileSystemBrowserModel;
class FileSystemDirectoryViewProxyModel;
class FileSystemFileViewProxyModel;
} // namespace Model

namespace View {
class FileSystemBrowserWidget : public QWidget {
  Q_OBJECT
public:
  explicit FileSystemBrowserWidget(
    QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  void setGame(const std::shared_ptr<Model::Game>& game);

  // For filtering files, eg. setFileTypeFilter("Model files", "mdl")
  void setFileTypeFilter(const QString& fileDescription, const QString& fileExtension);
  void clearFileTypeFilter();

  bool fileIsSelected() const;
  QString selectedFilePath() const;

signals:
  void fileChosen(const QString& path);

private slots:
  void onDirectoryActivated(const QModelIndex& index);
  void onFileActivated(const QModelIndex& index);
  void onFileSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void onDirectorySelectionChanged(
    const QItemSelection& selected, const QItemSelection& deselected);
  void updateFileFilter();
  void onFileChosen();
  void onCancelled();

private:
  void constructUI();
  void constructFileViewWidgets();
  void constructFileFilterWidgets();
  void connectSignals();

  void refresh();
  void setTableViewRoot(const QModelIndex& sourceRoot);

  QString getPathForTableViewItem(const QModelIndex& index) const;
  QString getSelectedFileTypeWildcardExt() const;

  std::shared_ptr<Model::Game> m_Game;

  QVBoxLayout* m_mainLayout = nullptr;
  QSplitter* m_fileSystemSplitter = nullptr;
  QTreeView* m_directoryView = nullptr;
  QTreeView* m_fileView = nullptr;

  QGridLayout* m_filterWidgetLayout = nullptr;
  QLineEdit* m_fileFilterTextBox = nullptr;
  QLineEdit* m_filePathTextBox = nullptr;
  QComboBox* m_fileTypeCombo = nullptr;

  QHBoxLayout* m_acceptButtonLayout = nullptr;
  QPushButton* m_chooseButton = nullptr;
  QPushButton* m_cancelButton = nullptr;

  Model::FileSystemBrowserModel* m_fsModel = nullptr;
  Model::FileSystemDirectoryViewProxyModel* m_directoryProxyModel = nullptr;
  Model::FileSystemFileViewProxyModel* m_fileProxyModel = nullptr;
};
} // namespace View
} // namespace TrenchBroom
