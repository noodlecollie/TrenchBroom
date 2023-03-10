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

#include <QDialog>
#include <memory>

class QVBoxLayout;

namespace TrenchBroom {
namespace Model {
class Game;
}

namespace View {
class FileSystemBrowserWidget;

class FileSystemBrowserDialog : public QDialog {
  Q_OBJECT
public:
  explicit FileSystemBrowserDialog(
    QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  void setGame(const std::shared_ptr<Model::Game>& game);

  // For filtering files, eg. setFileTypeFilter("Model files", "mdl")
  void setFileTypeFilter(const QString& fileDescription, const QString& fileExtension);
  void clearFileTypeFilter();

  bool fileIsSelected() const;
  QString selectedFilePath() const;

  static QString getFile(const std::shared_ptr<Model::Game>& game);

private slots:
  void browserWidgetChoseFile(const QString& filePath);

private:
  QVBoxLayout* m_layout = nullptr;
  FileSystemBrowserWidget* m_browserWidget = nullptr;
};
} // namespace View
} // namespace TrenchBroom
