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

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTableView>
#include <QTreeView>
#include <QVBoxLayout>
#include <QSplitter>

namespace TrenchBroom {
namespace View {
FileSystemBrowserWidget::FileSystemBrowserWidget(QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f) {
  QVBoxLayout* layout = new QVBoxLayout();

  QHBoxLayout* topLayout = new QHBoxLayout();

  m_FileSystemBucketComboBox = new QComboBox();
  topLayout->addWidget(m_FileSystemBucketComboBox);

  m_FilePathTextBox = new QLineEdit();
  topLayout->addWidget(m_FilePathTextBox);

  QSplitter* splitter = new QSplitter();

  m_FileSystemTreeView = new QTreeView();
  splitter->addWidget(m_FileSystemTreeView);

  m_FileSystemTableView = new QTableView();
  splitter->addWidget(m_FileSystemTableView);

  layout->addLayout(topLayout);
  layout->addWidget(splitter);
  setLayout(layout);
}
} // namespace View
} // namespace TrenchBroom
