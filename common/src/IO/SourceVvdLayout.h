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

#include <cstdint>

namespace TrenchBroom {
namespace IO {
namespace SourceVvdLayout {
#pragma pack(push, 1)

using Vector2D = float[2];
using Vector3D = float[3];

struct Header {
  int32_t id;                // MODEL_VERTEX_FILE_ID
  int32_t version;           // MODEL_VERTEX_FILE_VERSION
  int32_t checksum;          // same as studiohdr_t, ensures sync
  int32_t numLODs;           // num of valid lods
  int32_t numLODVertexes[8]; // num verts for desired root lod
  int32_t numFixups;         // num of vertexFileFixup_t
  int32_t fixupTableStart;   // offset from base to fixup table
  int32_t vertexDataStart;   // offset from base to vertex block
  int32_t tangentDataStart;  // offset from base to tangent block
};

struct BoneWeights {
  float weight[3];
  int8_t bone[3];
  uint8_t numbones;
};

struct Vertex {
  BoneWeights boneWeights;
  Vector3D position;
  Vector3D normal;
  Vector2D texCoOrd;
};

struct VertexFixUp {
  int32_t lod;
  int32_t sourceVertexID;
  int32_t numVertices;
};

#pragma pack(pop)
} // namespace SourceVvdLayout
} // namespace IO
} // namespace TrenchBroom
