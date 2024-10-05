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

#include "Polyhedron.h"
#include "Polyhedron_BrushGeometryPayload.h"
#include "Polyhedron_DefaultPayload.h"

namespace TrenchBroom::Model
{

extern template class Polyhedron_Vertex<
  double,
  DefaultPolyhedronPayload,
  DefaultPolyhedronPayload>;
extern template class Polyhedron_Vertex<double, BrushFacePayload, BrushVertexPayload>;

extern template class Polyhedron_Edge<
  double,
  DefaultPolyhedronPayload,
  DefaultPolyhedronPayload>;
extern template class Polyhedron_Edge<double, BrushFacePayload, BrushVertexPayload>;

extern template class Polyhedron_HalfEdge<
  double,
  DefaultPolyhedronPayload,
  DefaultPolyhedronPayload>;
extern template class Polyhedron_HalfEdge<double, BrushFacePayload, BrushVertexPayload>;

extern template class Polyhedron_Face<
  double,
  DefaultPolyhedronPayload,
  DefaultPolyhedronPayload>;
extern template class Polyhedron_Face<double, BrushFacePayload, BrushVertexPayload>;

extern template class Polyhedron<
  double,
  DefaultPolyhedronPayload,
  DefaultPolyhedronPayload>;
extern template class Polyhedron<double, BrushFacePayload, BrushVertexPayload>;

} // namespace TrenchBroom::Model
