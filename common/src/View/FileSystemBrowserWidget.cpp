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
#include "Model/FileSystemDirectoryViewProxyModel.h"
#include "Model/FileSystemFileViewProxyModel.h"
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
static constexpr int STRETCH_FACTOR_DIR_TREE = 1;
static constexpr int STRETCH_FACTOR_FILE_TABLE = 3;
static constexpr const char* const DEFAULT_FILE_FILTER_DESC = "All files";
static constexpr const char* const DEFAULT_FILE_FILTER_EXT = "";

FileSystemBrowserWidget::FileSystemBrowserWidget(QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f) {
  m_directoryProxyModel = new Model::FileSystemDirectoryViewProxyModel(this);
  m_fileProxyModel = new Model::FileSystemFileViewProxyModel(this);

  constructUI();
}

void FileSystemBrowserWidget::setGame(const std::shared_ptr<Model::Game>& game) {
  if (game == m_Game) {
    return;
  }

  m_Game = game;
  refresh();
}

void FileSystemBrowserWidget::setFileTypeFilter(
  const QString& fileDescription, const QString& fileExtension) {

  QString wildcardExtension = fileExtension.trimmed();

  if (wildcardExtension.isEmpty()) {
    wildcardExtension = "*";
  }

  m_fileTypeCombo->clear();
  m_fileTypeCombo->addItem(
    QString("%1 (*.%2)").arg(fileDescription).arg(wildcardExtension), QVariant(wildcardExtension));
}

void FileSystemBrowserWidget::clearFileTypeFilter() {
  setFileTypeFilter(DEFAULT_FILE_FILTER_DESC, DEFAULT_FILE_FILTER_EXT);
}

void FileSystemBrowserWidget::onDirectoryActivated(const QModelIndex& index) {
  if (!index.isValid()) {
    return;
  }

  setTableViewRoot(m_directoryProxyModel->mapToSource(index));
}

void FileSystemBrowserWidget::onFileActivated(const QModelIndex& index) {
  m_filePathTextBox->setText(getPathForTableViewItem(index));
}

void FileSystemBrowserWidget::onFileSelectionChanged(
  const QItemSelection& selected, const QItemSelection& deselected) {
  Q_UNUSED(deselected);

  // This should always only be one item - don't do anything if it's not.
  if (selected.count() == 1) {
    onFileActivated(selected.at(0).topLeft());
  }
}

void FileSystemBrowserWidget::onDirectorySelectionChanged(
  const QItemSelection& selected, const QItemSelection& deselected) {
  Q_UNUSED(deselected);

  // This should always only be one item - don't do anything if it's not.
  if (selected.count() == 1) {
    onDirectoryActivated(selected.at(0).topLeft());
  }
}

void FileSystemBrowserWidget::updateFileFilter() {
  m_fileProxyModel->setFilterWildcard(
    QString("%1*.%2").arg(m_fileFilterTextBox->text()).arg(getSelectedFileTypeWildcardExt()));
}

void FileSystemBrowserWidget::onFileChosen() {
  emit fileChosen(m_filePathTextBox->text());
}

void FileSystemBrowserWidget::onCancelled() {
  m_filePathTextBox->clear();
  m_fileView->clearSelection();
  emit fileChosen(QString());
}

void FileSystemBrowserWidget::constructUI() {
  m_mainLayout = new QVBoxLayout();
  m_mainLayout->setMargin(0);
  m_mainLayout->setSpacing(20);

  constructFileViewWidgets();
  constructFileFilterWidgets();
  connectSignals();
  clearFileTypeFilter();

  setLayout(m_mainLayout);
}

void FileSystemBrowserWidget::constructFileViewWidgets() {
  m_directoryView = new QTreeView();
  m_directoryView->setUniformRowHeights(true);
  m_directoryView->setSortingEnabled(true);
  m_directoryView->sortByColumn(0, Qt::AscendingOrder);
  m_directoryView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_directoryView->setSelectionMode(QAbstractItemView::SingleSelection);
  m_directoryView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_directoryView->setTextElideMode(Qt::ElideNone);

  m_fileView = new QTreeView();
  m_fileView->setItemsExpandable(false);
  m_fileView->setUniformRowHeights(true);
  m_fileView->setSortingEnabled(true);
  m_fileView->sortByColumn(0, Qt::AscendingOrder);
  m_fileView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_fileView->setSelectionMode(QAbstractItemView::SingleSelection);
  m_fileView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_fileView->setTextElideMode(Qt::ElideNone);

  m_fileSystemSplitter = new QSplitter();
  m_fileSystemSplitter->addWidget(m_directoryView);
  m_fileSystemSplitter->setStretchFactor(0, STRETCH_FACTOR_DIR_TREE);
  m_fileSystemSplitter->addWidget(m_fileView);
  m_fileSystemSplitter->setStretchFactor(1, STRETCH_FACTOR_FILE_TABLE);

  m_mainLayout->addWidget(m_fileSystemSplitter);
}

void FileSystemBrowserWidget::constructFileFilterWidgets() {
  m_filterWidgetLayout = new QGridLayout();
  m_filterWidgetLayout->setMargin(0);
  m_filterWidgetLayout->setSpacing(8);

  m_fileFilterTextBox = new QLineEdit();
  m_fileFilterTextBox->setPlaceholderText(tr("Filter"));
  m_filterWidgetLayout->addWidget(m_fileFilterTextBox, 0, 0);

  m_fileTypeCombo = new QComboBox();
  m_filterWidgetLayout->addWidget(m_fileTypeCombo, 0, 1);

  m_filePathTextBox = new QLineEdit();
  m_filePathTextBox->setReadOnly(true);
  m_filePathTextBox->setPlaceholderText(tr("Select a file above"));
  m_filterWidgetLayout->addWidget(m_filePathTextBox, 1, 0);

  m_acceptButtonLayout = new QHBoxLayout();
  m_acceptButtonLayout->setMargin(0);

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
    m_directoryView, &QTreeView::activated, this, &FileSystemBrowserWidget::onDirectoryActivated);

  connect(m_fileView, &QTreeView::activated, [this](const QModelIndex&) {
    onFileChosen();
  });

  connect(m_chooseButton, &QPushButton::clicked, this, &FileSystemBrowserWidget::onFileChosen);
  connect(m_cancelButton, &QPushButton::clicked, this, &FileSystemBrowserWidget::onCancelled);

  // These are dynamic connections because the signals have overloads and it confuses the compiler.
  connect(m_fileTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFileFilter()));
  connect(m_fileFilterTextBox, SIGNAL(textChanged(QString)), this, SLOT(updateFileFilter()));
}

void FileSystemBrowserWidget::refresh() {
  // Unhook the models before refreshing, in case live sorting
  // while the views are active causes performance issues.
  m_directoryView->setModel(nullptr);
  m_fileView->setModel(nullptr);

  m_fsModel = m_Game ? &m_Game->fileSystemBrowserModel() : nullptr;

  if (m_fsModel) {
    m_fsModel->reset();
  }

  m_directoryProxyModel->setSourceModel(m_fsModel);
  m_fileProxyModel->setSourceModel(m_fsModel);

  m_directoryView->setModel(m_directoryProxyModel);
  m_fileView->setModel(m_fileProxyModel);

  if (!m_fsModel) {
    return;
  }

  m_directoryView->setRootIndex(m_directoryProxyModel->mapFromSource(QModelIndex()));
  setTableViewRoot(m_fsModel->index(0, 0));

  // These must be conected here, as the model must have been set before they will work.
  connect(
    m_fileView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
    &FileSystemBrowserWidget::onFileSelectionChanged);

  connect(
    m_directoryView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
    &FileSystemBrowserWidget::onDirectorySelectionChanged);
}

void FileSystemBrowserWidget::setTableViewRoot(const QModelIndex& sourceRoot) {
  m_fileView->clearSelection();
  m_fileProxyModel->setRootForFiltering(sourceRoot);
  m_fileView->setRootIndex(m_fileProxyModel->mapFromSource(sourceRoot));
}

QString FileSystemBrowserWidget::getPathForTableViewItem(const QModelIndex& index) const {
  if (!index.isValid()) {
    return QString();
  }

  const QModelIndex sourceIndex = m_fileProxyModel->mapToSource(index);
  return m_fsModel->data(sourceIndex, Model::FileSystemBrowserModel::DataRole::ROLE_FULL_PATH)
    .toString();
}

QString FileSystemBrowserWidget::getSelectedFileTypeWildcardExt() const {
  const QString wildcardExt = m_fileTypeCombo->currentData().toString();
  return (!wildcardExt.isEmpty()) ? wildcardExt : "*";
}
} // namespace View
} // namespace TrenchBroom
