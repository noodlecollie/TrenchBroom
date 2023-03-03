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

#include "View/FileSystemBrowserDialog.h"
#include "View/FileSystemBrowserWidget.h"
#include <QVBoxLayout>

namespace TrenchBroom {
namespace View {
FileSystemBrowserDialog::FileSystemBrowserDialog(QWidget* parent, Qt::WindowFlags f)
  : QDialog(parent, f) {
  m_layout = new QVBoxLayout();

  m_browserWidget = new FileSystemBrowserWidget();
  m_layout->addWidget(m_browserWidget);

  connect(
    m_browserWidget, &FileSystemBrowserWidget::fileChosen, this,
    &FileSystemBrowserDialog::browserWidgetChoseFile);

  setLayout(m_layout);
  setSizeGripEnabled(true);
}

void FileSystemBrowserDialog::setGame(const std::shared_ptr<Model::Game>& game) {
  m_browserWidget->setGame(game);
}

bool FileSystemBrowserDialog::fileIsSelected() const {
  return m_browserWidget->fileIsSelected();
}

QString FileSystemBrowserDialog::selectedFilePath() const {
  return m_browserWidget->selectedFilePath();
}

QString FileSystemBrowserDialog::getFile(const std::shared_ptr<Model::Game>& game) {
  FileSystemBrowserDialog dialog;

  dialog.setWindowTitle(tr("Select a file"));
  dialog.setGame(game);

  const int resultCode = dialog.exec();
  return resultCode == DialogCode::Accepted ? dialog.selectedFilePath() : QString();
}

void FileSystemBrowserDialog::browserWidgetChoseFile(const QString& filePath) {
  if (!filePath.isEmpty()) {
    accept();
  } else {
    reject();
  }
}
} // namespace View
} // namespace TrenchBroom
