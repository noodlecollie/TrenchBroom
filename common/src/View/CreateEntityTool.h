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

#include "View/Tool.h"

#include "vm/bbox.h" // IWYU pragma: keep
#include "vm/forward.h"

#include <memory>
#include <string>

namespace TrenchBroom::Model
{
class EntityNode;
class PickResult;
} // namespace TrenchBroom::Model

namespace TrenchBroom::View
{
class MapDocument;

class CreateEntityTool : public Tool
{
private:
  std::weak_ptr<MapDocument> m_document;
  Model::EntityNode* m_entity = nullptr;
  vm::bbox3d m_referenceBounds;

public:
  explicit CreateEntityTool(std::weak_ptr<MapDocument> document);

  bool createEntity(const std::string& classname);
  void removeEntity();
  void commitEntity();

  void updateEntityPosition2D(const vm::ray3d& pickRay);
  void updateEntityPosition3D(
    const vm::ray3d& pickRay, const Model::PickResult& pickResult);
};

} // namespace TrenchBroom::View
