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

#include "GridRenderer.h"

#include "PreferenceManager.h"
#include "Preferences.h"
#include "render/ActiveShader.h"
#include "render/OrthographicCamera.h"
#include "render/PrimType.h"
#include "render/RenderContext.h"
#include "render/Shaders.h"

#include "vm/bbox.h"
#include "vm/vec.h"

namespace tb::render
{
GridRenderer::GridRenderer(
  const OrthographicCamera& camera, const vm::bbox3d& worldBounds)
  : m_vertexArray(VertexArray::move(vertices(camera, worldBounds)))
{
}

std::vector<GridRenderer::Vertex> GridRenderer::vertices(
  const OrthographicCamera& camera, const vm::bbox3d& worldBounds)
{
  const auto& viewport = camera.zoomedViewport();
  const auto w = float(viewport.width) / 2.0f;
  const auto h = float(viewport.height) / 2.0f;

  const auto& p = camera.position();
  switch (vm::find_abs_max_component(camera.direction()))
  {
  case vm::axis::x:
    return {
      Vertex{vm::vec3f{float(worldBounds.min.x()), p.y() - w, p.z() - h}},
      Vertex{vm::vec3f{float(worldBounds.min.x()), p.y() - w, p.z() + h}},
      Vertex{vm::vec3f{float(worldBounds.min.x()), p.y() + w, p.z() + h}},
      Vertex{vm::vec3f{float(worldBounds.min.x()), p.y() + w, p.z() - h}}};
  case vm::axis::y:
    return {
      Vertex{vm::vec3f{p.x() - w, float(worldBounds.max.y()), p.z() - h}},
      Vertex{vm::vec3f{p.x() - w, float(worldBounds.max.y()), p.z() + h}},
      Vertex{vm::vec3f{p.x() + w, float(worldBounds.max.y()), p.z() + h}},
      Vertex{vm::vec3f{p.x() + w, float(worldBounds.max.y()), p.z() - h}}};
  case vm::axis::z:
    return {
      Vertex{vm::vec3f{p.x() - w, p.y() - h, float(worldBounds.min.z())}},
      Vertex{vm::vec3f{p.x() - w, p.y() + h, float(worldBounds.min.z())}},
      Vertex{vm::vec3f{p.x() + w, p.y() + h, float(worldBounds.min.z())}},
      Vertex{vm::vec3f{p.x() + w, p.y() - h, float(worldBounds.min.z())}}};
  default:
    // Should not happen.
    return {};
  }
}

void GridRenderer::doPrepareVertices(VboManager& vboManager)
{
  m_vertexArray.prepare(vboManager);
}

void GridRenderer::doRender(RenderContext& renderContext)
{
  if (renderContext.showGrid())
  {
    const auto& camera = renderContext.camera();

    auto shader = ActiveShader{renderContext.shaderManager(), Shaders::Grid2DShader};
    shader.set("Normal", -camera.direction());
    shader.set("RenderGrid", renderContext.showGrid());
    shader.set("GridSize", static_cast<float>(renderContext.gridSize()));
    shader.set("GridAlpha", pref(Preferences::GridAlpha));
    shader.set("GridColor", pref(Preferences::GridColor2D));
    shader.set("CameraZoom", camera.zoom());

    m_vertexArray.render(PrimType::Quads);
  }
}

} // namespace tb::render
