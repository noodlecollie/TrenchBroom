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

#include "View/FilePickerPropertyEditor.h"

#include "Ensure.h"
#include "View/FileSystemBrowserDialog.h"
#include "View/MapDocument.h"
#include "kdl/memory_utils.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

namespace TrenchBroom {
namespace View {
FilePickerPropertyEditor::FilePickerPropertyEditor(
  const std::weak_ptr<MapDocument>& document, QWidget* parent)
  : QWidget(parent)
  , m_document(document) {
  m_layout = new QHBoxLayout();
  m_layout->setMargin(0);
  m_layout->setSpacing(0);

  m_lineEdit = new QLineEdit();
  m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_layout->addWidget(m_lineEdit);

  m_pickerButton = new QPushButton();
  m_pickerButton->setText(tr("Choose"));
  m_layout->addWidget(m_pickerButton);

  m_fsDialog = new FileSystemBrowserDialog(this);
  m_fsDialog->setWindowTitle(tr("Select a file"));
  m_fsDialog->setModal(true);

  setLayout(m_layout);

  connect(m_pickerButton, &QPushButton::clicked, this, &FilePickerPropertyEditor::openPickerDialog);

  connect(
    m_fsDialog, &FileSystemBrowserDialog::accepted, this,
    &FilePickerPropertyEditor::pickerDialogAccepted);

  connect(
    m_fsDialog, &FileSystemBrowserDialog::rejected, this,
    &FilePickerPropertyEditor::pickerDialogRejected);
}

QString FilePickerPropertyEditor::filePath() const {
  return m_lineEdit->text();
}

void FilePickerPropertyEditor::setFilePath(const QString& path) {
  m_lineEdit->setText(path);
}

void FilePickerPropertyEditor::setFileTypeFilter(
  const QString& fileDescription, const QString& fileExtension) {
  m_fsDialog->setFileTypeFilter(fileDescription, fileExtension);
}

void FilePickerPropertyEditor::clearFileTypeFilter() {
  m_fsDialog->clearFileTypeFilter();
}

void FilePickerPropertyEditor::openPickerDialog() {
  if (isLockedForPicking()) {
    return;
  }

  lockForPicking();

  m_fsDialog->setGame(m_documentSharedPtr->game());
  m_fsDialog->show();
}

void FilePickerPropertyEditor::pickerDialogAccepted() {
  m_lineEdit->setText(m_fsDialog->selectedFilePath());
  unlockAfterPicking();
}

void FilePickerPropertyEditor::pickerDialogRejected() {
  // Don't modify the contents of the line edit.
  unlockAfterPicking();
}

void FilePickerPropertyEditor::lockForPicking() {
  m_documentSharedPtr = kdl::mem_lock(m_document);
}

void FilePickerPropertyEditor::unlockAfterPicking() {
  m_documentSharedPtr.reset();
}

bool FilePickerPropertyEditor::isLockedForPicking() {
  return m_documentSharedPtr.operator bool();
}
} // namespace View
} // namespace TrenchBroom
