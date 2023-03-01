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
#include "Model/FileSystemBrowserTableProxyModel.h"
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
#include <QtDebug>

namespace TrenchBroom {
namespace View {
static constexpr int STRETCH_FACTOR_DIR_TREE = 1;
static constexpr int STRETCH_FACTOR_FILE_TABLE = 3;

FileSystemBrowserWidget::FileSystemBrowserWidget(QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f) {
  m_treeProxyModel = new Model::FileSystemBrowserTreeProxyModel(this);
  m_tableProxyModel = new Model::FileSystemBrowserTableProxyModel(this);

  constructUI();
}

void FileSystemBrowserWidget::setGame(const std::shared_ptr<Model::Game>& game) {
  if (game == m_Game) {
    return;
  }

  m_Game = game;
  refresh();
}

void FileSystemBrowserWidget::onDirectoryActivated(const QModelIndex& index) {
  if (!index.isValid()) {
    return;
  }

  setTableViewRoot(m_treeProxyModel->mapToSource(index));
}

void FileSystemBrowserWidget::onFileActivated(const QModelIndex& index) {
  m_filePathTextBox->setText(getPathForTableViewItem(index));
}

void FileSystemBrowserWidget::constructUI() {
  m_mainLayout = new QVBoxLayout();

  constructFileViewWidgets();
  constructFileFilterWidgets();
  connectSignals();

  setLayout(m_mainLayout);
}

void FileSystemBrowserWidget::constructFileViewWidgets() {
  m_fileSystemTreeView = new QTreeView();
  m_fileSystemTreeView->setSortingEnabled(true);
  m_fileSystemTreeView->sortByColumn(0, Qt::AscendingOrder);
  m_fileSystemTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_fileSystemTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
  m_fileSystemTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_fileSystemTreeView->setTextElideMode(Qt::ElideNone);

  m_fileSystemTableView = new QTableView();
  m_fileSystemTableView->horizontalHeader()->setStretchLastSection(true);
  m_fileSystemTableView->verticalHeader()->setVisible(false);
  m_fileSystemTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_fileSystemTableView->setSelectionMode(QAbstractItemView::SingleSelection);
  m_fileSystemTableView->setSortingEnabled(true);
  m_fileSystemTableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_fileSystemTableView->setTextElideMode(Qt::ElideNone);

  m_fileSystemSplitter = new QSplitter();
  m_fileSystemSplitter->addWidget(m_fileSystemTreeView);
  m_fileSystemSplitter->setStretchFactor(0, STRETCH_FACTOR_DIR_TREE);
  m_fileSystemSplitter->addWidget(m_fileSystemTableView);
  m_fileSystemSplitter->setStretchFactor(1, STRETCH_FACTOR_FILE_TABLE);

  m_mainLayout->addWidget(m_fileSystemSplitter);
}

void FileSystemBrowserWidget::constructFileFilterWidgets() {
  m_filterWidgetLayout = new QGridLayout();

  m_fileFilterTextBox = new QLineEdit();
  m_fileFilterTextBox->setPlaceholderText(tr("Filter"));
  m_filterWidgetLayout->addWidget(m_fileFilterTextBox, 0, 0);

  m_fileTypeCombo = new QComboBox();
  m_fileTypeCombo->addItem(tr("File type")); // FIXME
  m_filterWidgetLayout->addWidget(m_fileTypeCombo, 0, 1);

  m_filePathTextBox = new QLineEdit();
  m_filePathTextBox->setReadOnly(true);
  m_filePathTextBox->setPlaceholderText(tr("Select a file"));
  m_filterWidgetLayout->addWidget(m_filePathTextBox, 1, 0);

  m_acceptButtonLayout = new QHBoxLayout();

  m_chooseButton = new QPushButton();
  m_chooseButton->setText(tr("Choose"));
  m_acceptButtonLayout->addWidget(m_chooseButton);

  m_cancelButton = new QPushButton();
  m_cancelButton->setText(tr("Cancel"));
  m_acceptButtonLayout->addWidget(m_cancelButton);

  m_filterWidgetLayout->addLayout(m_acceptButtonLayout, 1, 1);

  m_mainLayout->addLayout(m_filterWidgetLayout);
}

void FileSystemBrowserWidget::connectSignals() {
  connect(
    m_fileSystemTreeView, &QTreeView::activated, this,
    &FileSystemBrowserWidget::onDirectoryActivated);

  connect(
    m_fileSystemTableView, &QTableView::activated, this, &FileSystemBrowserWidget::onFileActivated);
}

void FileSystemBrowserWidget::refresh() {
  // Unhook the models before refreshing, in case live sorting
  // while the views are active causes performance issues.
  m_fileSystemTreeView->setModel(nullptr);
  m_fileSystemTableView->setModel(nullptr);

  m_fsModel = m_Game ? &m_Game->fileSystemBrowserModel() : nullptr;

  if (m_fsModel) {
    m_fsModel->reset();
  }

  m_treeProxyModel->setSourceModel(m_fsModel);
  m_tableProxyModel->setSourceModel(m_fsModel);

  m_fileSystemTreeView->setModel(m_treeProxyModel);
  m_fileSystemTableView->setModel(m_tableProxyModel);

  if (!m_fsModel) {
    return;
  }

  m_fileSystemTreeView->setRootIndex(m_treeProxyModel->mapFromSource(QModelIndex()));
  setTableViewRoot(m_fsModel->index(0, 0));
}

void FileSystemBrowserWidget::setTableViewRoot(const QModelIndex& sourceRoot) {
  m_tableProxyModel->setRootForFiltering(sourceRoot);
  m_fileSystemTableView->setRootIndex(m_tableProxyModel->mapFromSource(sourceRoot));
}

QString FileSystemBrowserWidget::getPathForTableViewItem(const QModelIndex& index) const {
  if (!index.isValid()) {
    return QString();
  }

  const QModelIndex sourceIndex = m_tableProxyModel->mapToSource(index);
  return m_fsModel->data(sourceIndex, Model::FileSystemBrowserModel::DataRole::ROLE_FULL_PATH)
    .toString();
}
} // namespace View
} // namespace TrenchBroom
