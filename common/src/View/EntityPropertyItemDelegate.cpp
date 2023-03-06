/*
 Copyright (C) 2010-2019 Kristian Duske

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

#include "EntityPropertyItemDelegate.h"

#include "Model/EntityProperties.h"
#include "View/EntityPropertyModel.h"
#include "View/EntityPropertyTable.h"
#include "View/FilePickerPropertyEditor.h"
#include "View/MapDocument.h"

#include <string>
#include <vector>

#include <QCompleter>
#include <QDebug>
#include <QEvent>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QTimer>

namespace TrenchBroom {
namespace View {
EntityPropertyItemDelegate::EntityPropertyItemDelegate(
  std::weak_ptr<MapDocument> document, EntityPropertyTable* table, const EntityPropertyModel* model,
  const QSortFilterProxyModel* proxyModel, QWidget* parent)
  : QStyledItemDelegate(parent)
  , m_document(document)
  , m_table(table)
  , m_model(model)
  , m_proxyModel(proxyModel) {}

QWidget* EntityPropertyItemDelegate::createEditor(
  QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  // The alternative to jumping in here would be to register a new type for certain properties, and
  // use QItemEditorFactory::registerEditor() to specify the type of widget to create for this new
  // type. However, I don't think that would quite be what we'd want here, since the property isn't
  // a different type per se to any other property - we just use a picker to fill in the data.
  // Therefore, we check here to see if the key is something we should use a picker for, and swap
  // out the editor widget if we need to.
  const QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
  if (m_model->propertyKey(sourceIndex.row()) == Model::EntityPropertyKeys::StudioModel) {
    return new FilePickerPropertyEditor(m_document, parent);
  }

  auto* editor = QStyledItemDelegate::createEditor(parent, option, index);
  auto* lineEdit = dynamic_cast<QLineEdit*>(editor);
  if (lineEdit != nullptr) {
    setupCompletions(lineEdit, index);
  }
  return editor;
}

void EntityPropertyItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
  QStyledItemDelegate::setEditorData(editor, index);

  // show the completions immediately when the editor is opened if the editor's text is empty
  auto* lineEdit = dynamic_cast<QLineEdit*>(editor);
  if (lineEdit != nullptr) {
    // Delay to work around https://github.com/TrenchBroom/TrenchBroom/issues/3082
    // Briefly, when typing the first letter of the text you want to enter to open the cell editor,
    // when setEditorData() runs, the letter has not been inserted into the QLineEdit yet.
    // Opening the completion popup and then typing the letter causes the editor to close, which is
    // issue #3082 and quite annoying. Only happens on Linux.
    QTimer::singleShot(0, lineEdit, [lineEdit]() {
      const QString text = lineEdit->text();
      if (text.isEmpty()) {
        QCompleter* completer = lineEdit->completer();
        if (completer != nullptr) {
          completer->setCompletionPrefix("");
          completer->complete();
        }
      }
    });
  }
}

void EntityPropertyItemDelegate::setupCompletions(
  QLineEdit* lineEdit, const QModelIndex& index) const {
  auto* completer = new QCompleter(getCompletions(index), lineEdit);
  completer->setCaseSensitivity(Qt::CaseInsensitive);
  completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
  lineEdit->setCompleter(completer);

  connect(
    completer, QOverload<const QString&>::of(&QCompleter::activated), this,
    [this, lineEdit](const QString& /* value */) {
      m_table->finishEditing(lineEdit);
    });

  connect(lineEdit, &QLineEdit::returnPressed, this, [this, lineEdit, completer]() {
    if (completer->popup()->isVisible()) {
      m_table->finishEditing(lineEdit);
    }
  });
}

QStringList EntityPropertyItemDelegate::getCompletions(const QModelIndex& index) const {
  auto completions = m_model->getCompletions(m_proxyModel->mapToSource(index));
  completions.sort(Qt::CaseInsensitive);
  return completions;
}
} // namespace View
} // namespace TrenchBroom
