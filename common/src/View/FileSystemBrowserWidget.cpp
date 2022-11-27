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

#include "View/FileSystemBrowserWidget.h"

#include "Model/FileSystemBrowserModel.h"
#include "Model/FileSystemBrowserTreeProxyModel.h"
#include "Model/Game.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>
#include <QVBoxLayout>

namespace TrenchBroom {
namespace View {
FileSystemBrowserWidget::FileSystemBrowserWidget(QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f) {
  m_treeProxyModel = new Model::FileSystemBrowserTreeProxyModel(this);

  QVBoxLayout* layout = new QVBoxLayout();

  QHBoxLayout* topLayout = new QHBoxLayout();

  m_filePathTextBox = new QLineEdit();
  topLayout->addWidget(m_filePathTextBox);

  QSplitter* splitter = new QSplitter();

  m_fileSystemTreeView = new QTreeView();
  m_fileSystemTreeView->setSortingEnabled(true);
  m_fileSystemTreeView->sortByColumn(0, Qt::AscendingOrder);
  splitter->addWidget(m_fileSystemTreeView);
  splitter->setStretchFactor(0, 1);

  m_fileSystemTableView = new QTableView();
  splitter->addWidget(m_fileSystemTableView);
  splitter->setStretchFactor(1, 2);

  m_fileSystemTableView->horizontalHeader()->setStretchLastSection(true);
  m_fileSystemTableView->verticalHeader()->setVisible(false);
  m_fileSystemTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_fileSystemTableView->setSelectionMode(QAbstractItemView::SingleSelection);

  layout->addLayout(topLayout);
  layout->addWidget(splitter);
  setLayout(layout);
}

void FileSystemBrowserWidget::setGame(const std::shared_ptr<Model::Game>& game) {
  if (game == m_Game) {
    return;
  }

  m_Game = game;
  refresh();
}

void FileSystemBrowserWidget::refresh() {
  // Unhook the models before refreshing, in case live sorting
  // while the views are active causes performance issues.
  m_fileSystemTreeView->setModel(nullptr);
  m_fileSystemTableView->setModel(nullptr);

  Model::FileSystemBrowserModel* fsModel = m_Game ? &m_Game->fileSystemBrowserModel() : nullptr;

  if (fsModel) {
    fsModel->reset();
  }

  m_treeProxyModel->setSourceModel(fsModel);

  m_fileSystemTreeView->setModel(m_treeProxyModel);
  m_fileSystemTableView->setModel(fsModel);
}
} // namespace View
} // namespace TrenchBroom
