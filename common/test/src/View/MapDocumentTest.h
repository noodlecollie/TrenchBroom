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

#include "Model/MapFormat.h"
#include "View/MapDocument.h"

#include <functional>
#include <memory>
#include <string>

namespace tb::assets
{
class BrushEntityDefinition;
class PointEntityDefinition;
} // namespace tb::assets

namespace tb::Model
{
class Brush;
class PatchNode;
class TestGame;
} // namespace tb::Model

namespace tb::View
{

class MapDocumentTest
{
private:
  Model::MapFormat m_mapFormat;

protected:
  std::shared_ptr<Model::TestGame> game;
  std::shared_ptr<MapDocument> document;
  assets::PointEntityDefinition* m_pointEntityDef = nullptr;
  assets::BrushEntityDefinition* m_brushEntityDef = nullptr;

protected:
  MapDocumentTest();
  explicit MapDocumentTest(Model::MapFormat mapFormat);

private:
  void SetUp();

protected:
  virtual ~MapDocumentTest();

public:
  Model::BrushNode* createBrushNode(
    const std::string& materialName = "material",
    const std::function<void(Model::Brush&)>& brushFunc = [](Model::Brush&) {}) const;
  Model::PatchNode* createPatchNode(const std::string& materialName = "material") const;
};

class ValveMapDocumentTest : public MapDocumentTest
{
protected:
  ValveMapDocumentTest();
};

class Quake3MapDocumentTest : public MapDocumentTest
{
public:
  Quake3MapDocumentTest();
};

} // namespace tb::View
