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

#pragma once

#include "View/ToolController.h"

#include "vm/vec.h" // IWYU pragma: keep

#include <memory>

namespace TrenchBroom::View
{

class DrawShapeTool;
class GestureTracker;
class MapDocument;

class DrawShapeToolController3D : public ToolController
{
private:
  DrawShapeTool& m_tool;
  std::weak_ptr<MapDocument> m_document;

  vm::vec3d m_initialPoint;

public:
  DrawShapeToolController3D(DrawShapeTool& tool, std::weak_ptr<MapDocument> document);

private:
  Tool& tool() override;
  const Tool& tool() const override;

  std::unique_ptr<GestureTracker> acceptMouseDrag(const InputState& inputState) override;

  bool cancel() override;
};

} // namespace TrenchBroom::View
