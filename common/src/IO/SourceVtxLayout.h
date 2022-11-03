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
namespace SourceVtxLayout {
#pragma pack(push, 1)

static constexpr uint8_t STRIP_IS_TRILIST = (1 << 0);
static constexpr uint8_t STRIP_IS_TRISTRIP = (1 << 1);

struct Header {
  // file version as defined by OPTIMIZED_MODEL_FILE_VERSION (currently 7)
  int32_t version;

  // hardware params that affect how the model is to be optimized.
  int32_t vertCacheSize;
  uint16_t maxBonesPerStrip;
  uint16_t maxBonesPerTri;
  int32_t maxBonesPerVert;

  // must match checkSum in the .mdl
  int32_t checksum;

  int32_t numLODs;

  // Offset to materialReplacementList Array. one of these for each LOD, 8 in total
  int32_t materialReplacementListOffset;

  // Defines the size and location of the body part array
  int32_t numBodyParts;
  int32_t bodyPartOffset;
};

struct BodyPart {
  int32_t numModels;
  int32_t modelOffset;
};

struct Model {
  int32_t numLODs;
  int32_t lodOffset;
};

struct LOD {
  int32_t numMeshes;
  int32_t meshOffset;
  float switchPoint;
};

struct Mesh {
  int32_t numStripGroups;
  int32_t stripGroupHeaderOffset;
  uint8_t flags;
};

struct StripGroup {
  // These are the arrays of all verts and indices for this mesh. strips index into this.
  int32_t numVerts;
  int32_t vertOffset;

  int32_t numIndices;
  int32_t indexOffset;

  int32_t numStrips;
  int32_t stripOffset;

  uint8_t flags;
};

struct Strip {
  // indexOffset offsets into the mesh's index array.
  int32_t numIndices;
  int32_t indexOffset;

  // vertexOffset offsets into the mesh's vert array.
  int32_t numVerts;
  int32_t vertOffset;

  // use this to enable/disable skinning.
  // May decide to put all with 1 bone in a different strip
  // than those that need skinning.
  int16_t numBones;

  uint8_t flags;

  int32_t numBoneStateChanges;
  int32_t boneStateChangeOffset;
};

struct Vertex {
  uint8_t boneWeightIndex[3];
  uint8_t numBones;

  uint16_t origMeshVertID;

  int8_t boneID[3];
};

struct Index {
  uint16_t value;
};

#pragma pack(pop)
} // namespace SourceVtxLayout
} // namespace IO
} // namespace TrenchBroom
