/*
 Copyright (C) 2010-2014 Kristian Duske

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

#include "View/DebugFileSystemBrowserWindow.h"

#include "View/FileSystemBrowserWidget.h"
#include <QLineEdit>
#include <QVBoxLayout>

namespace TrenchBroom {
namespace View {
DebugFileSystemBrowserWindow::DebugFileSystemBrowserWindow(QWidget* parent)
  : QDialog(parent) {
  setWindowTitle(tr("File System"));

  QVBoxLayout* layout = new QVBoxLayout();

  m_BrowserWidget = new FileSystemBrowserWidget();
  layout->addWidget(m_BrowserWidget);

  setLayout(layout);

  connect(
    m_BrowserWidget, &FileSystemBrowserWidget::fileChosen, this,
    &DebugFileSystemBrowserWindow::onFileChosen);
}

DebugFileSystemBrowserWindow::~DebugFileSystemBrowserWindow() {}

void DebugFileSystemBrowserWindow::setGame(const std::shared_ptr<Model::Game>& game) {
  m_BrowserWidget->setGame(game);
}

void DebugFileSystemBrowserWindow::onFileChosen(const QString& path) {
  qDebug() << "DebugFileSystemBrowserWindow file chosen:" << path;
}
} // namespace View
} // namespace TrenchBroom
