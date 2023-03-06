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

  setLayout(m_layout);

  connect(m_pickerButton, &QPushButton::clicked, this, &FilePickerPropertyEditor::openPickerDialog);
}

QString FilePickerPropertyEditor::filePath() const {
  return m_lineEdit->text();
}

void FilePickerPropertyEditor::setFilePath(const QString& path) {
  m_lineEdit->setText(path);
}

void FilePickerPropertyEditor::openPickerDialog() {
  // m_documentSharedPtr = kdl::mem_lock(m_document);
  // const QString path = FileSystemBrowserDialog::getFile(m_documentSharedPtr->game());

  // if (!path.isEmpty()) {
  //   m_lineEdit->setText(path);
  // }

  // m_documentSharedPtr.reset();

  // TODO: The above freezes up the application, and also doesn't submit the data once the editing
  // is finished.
}
} // namespace View
} // namespace TrenchBroom
