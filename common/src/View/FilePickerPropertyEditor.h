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

class QHBoxLayout;
class QPushButton;
class QLineEdit;

namespace TrenchBroom {
namespace View {
class MapDocument;
class FileSystemBrowserDialog;

class FilePickerPropertyEditor : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString filePath READ filePath WRITE setFilePath USER true)

public:
  explicit FilePickerPropertyEditor(
    const std::weak_ptr<MapDocument>& document, QWidget* parent = nullptr);

  QString filePath() const;
  void setFilePath(const QString& path);

  // For filtering files, eg. setFileTypeFilter("Model files", "mdl")
  void setFileTypeFilter(const QString& fileDescription, const QString& fileExtension);
  void clearFileTypeFilter();

private slots:
  void openPickerDialog();
  void pickerDialogAccepted();
  void pickerDialogRejected();

private:
  void lockForPicking();
  void unlockAfterPicking();
  bool isLockedForPicking();

  std::weak_ptr<MapDocument> m_document;

  // For when the picker dialogue is open - we want to keep a strong reference to the document at
  // this point.
  std::shared_ptr<MapDocument> m_documentSharedPtr;

  QHBoxLayout* m_layout = nullptr;
  QLineEdit* m_lineEdit = nullptr;
  QPushButton* m_pickerButton = nullptr;
  FileSystemBrowserDialog* m_fsDialog = nullptr;
};
} // namespace View
} // namespace TrenchBroom
