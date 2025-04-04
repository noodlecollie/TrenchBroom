/*
 Copyright (C) 2010 Kristian Duske

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

#include "FormWithSectionsLayout.h"

#include <QBoxLayout>
#include <QLabel>

#include "ui/QtUtils.h"
#include "ui/ViewConstants.h"

namespace tb::ui
{

namespace
{
constexpr auto FieldLeftMargin = 3 * LayoutConstants::WideHMargin;
}

void FormWithSectionsLayout::addSection(const QString& title, const QString& info)
{
  const auto topMargin = rowCount() > 0 ? 2 * LayoutConstants::WideVMargin : 0;

  auto* titleLayout = new QVBoxLayout{};
  titleLayout->setContentsMargins(0, topMargin, LayoutConstants::WideHMargin, 0);
  titleLayout->setSpacing(0);
  titleLayout->addWidget(makeEmphasized(new QLabel{title}));

  if (!info.isEmpty())
  {
    auto* infoLabel = new QLabel{info};
    infoLabel->setWordWrap(true);
    makeInfo(infoLabel);

    titleLayout->addSpacing(LayoutConstants::NarrowVMargin);
    titleLayout->addWidget(infoLabel);
  }

  titleLayout->addSpacing(LayoutConstants::MediumVMargin);

  QFormLayout::addRow(titleLayout);
}

void FormWithSectionsLayout::addRow(QWidget* label, QWidget* field)
{
  insertRow(rowCount(), label, field);
}

void FormWithSectionsLayout::addRow(QWidget* label, QLayout* field)
{
  insertRow(rowCount(), label, field);
}

void FormWithSectionsLayout::addRow(const QString& labelText, QWidget* field)
{
  insertRow(rowCount(), labelText, field);
}

void FormWithSectionsLayout::addRow(const QString& labelText, QLayout* field)
{
  insertRow(rowCount(), labelText, field);
}

void FormWithSectionsLayout::addRow(QWidget* field)
{
  insertRow(rowCount(), field);
}

void FormWithSectionsLayout::addRow(QLayout* field)
{
  insertRow(rowCount(), field);
}

void FormWithSectionsLayout::insertRow(const int row, QWidget* label, QWidget* field)
{
  auto* labelLayout = new QHBoxLayout{};
  labelLayout->setContentsMargins(FieldLeftMargin, 0, 0, 0);
  labelLayout->addWidget(label);

  auto* fieldLayout = new QHBoxLayout{};
  fieldLayout->setContentsMargins(0, 0, LayoutConstants::WideHMargin, 0);
  fieldLayout->addWidget(field);

  setLayout(row, QFormLayout::LabelRole, labelLayout);
  setLayout(row, QFormLayout::FieldRole, fieldLayout);
}

void FormWithSectionsLayout::insertRow(const int row, QWidget* label, QLayout* field)
{
  auto* labelLayout = new QHBoxLayout{};
  labelLayout->setContentsMargins(FieldLeftMargin, 0, 0, 0);
  labelLayout->addWidget(label);

  auto* fieldLayout = new QHBoxLayout{};
  fieldLayout->setContentsMargins(0, 0, LayoutConstants::WideHMargin, 0);
  fieldLayout->addLayout(field);

  setLayout(row, QFormLayout::LabelRole, labelLayout);
  setLayout(row, QFormLayout::FieldRole, fieldLayout);
}

void FormWithSectionsLayout::insertRow(
  const int row, const QString& labelText, QWidget* field)
{
  insertRow(row, new QLabel{labelText}, field);
}

void FormWithSectionsLayout::insertRow(
  const int row, const QString& labelText, QLayout* field)
{
  insertRow(row, new QLabel{labelText}, field);
}

void FormWithSectionsLayout::insertRow(int row, QWidget* field)
{
  auto* layout = new QHBoxLayout{};
  layout->setContentsMargins(FieldLeftMargin, 0, LayoutConstants::WideHMargin, 0);
  layout->addWidget(field);
  QFormLayout::insertRow(row, layout);
}

void FormWithSectionsLayout::insertRow(int row, QLayout* field)
{
  auto* layout = new QHBoxLayout{};
  layout->setContentsMargins(FieldLeftMargin, 0, LayoutConstants::WideHMargin, 0);
  layout->addLayout(field);
  QFormLayout::insertRow(row, layout);
}

} // namespace tb::ui
