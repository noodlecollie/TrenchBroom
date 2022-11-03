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

#include "Exceptions.h"
#include "IO/File.h"
#include "IO/SourceMdlFormatUtils.h"
#include "IO/SourceVtxDataAccessor.h"
#include "IO/SourceVtxLayout.h"
#include <vector>

namespace TrenchBroom {
namespace IO {
SourceVtxDataAccessor::SourceVtxDataAccessor(const std::shared_ptr<File>& file)
  : m_file(file)
  , m_reader(m_file->reader().buffer()) {
  m_reader.read(reinterpret_cast<char*>(&m_header), sizeof(m_header));
}

void SourceVtxDataAccessor::validate(SourceMdlLayout::Header mdlHeader) const {
  if (!isSourceVtxVersion(m_header.version)) {
    throw AssetException("Unsupported Source VTX version: " + std::to_string(m_header.version));
  }

  if (m_header.checksum != mdlHeader.checksum) {
    throw AssetException(
      "Source VTX file checksum " + std::to_string(m_header.checksum) +
      " did not match MDL file checksum " + std::to_string(mdlHeader.checksum));
  }
}

// This is annoyingly complicated. Here's a quick summary of what's going on conceptually here:
// Based on the body index and set of body part in an MDL file, we get a set of meshes to draw.
// Each mesh defines its own set of vertices with attributes (position, texture co-ordinates, etc.).
// However, the MDL file itself does not define how these vertices are used to draw triangles -
// that's the job of the VTX files, which are optimised for the target hardware/API/whatever that's
// going to be doing the drawing.
// We have to parse a VTX file and select the list of meshes, in the same way that this is done for
// the MDL file. Each VTX mesh contains one or more "strip groups", where a strip group defines a
// set of vertices and indices that are specific to that group, and are completely independent from
// the vertices in the MDL file. However, each strip vertex holds an index which maps onto one of
// the vertices in the MDL file, and these strip vertices themselves are indexed by the strip
// indices to specify the exact order to draw each triangle in the mesh... Got all that?
// Given the indices for the body part, submodel, LOD and mesh that you want, this function runs
// through all the strips groups and strips for that mesh, and condenses down all the individual
// vertices and indices. For each strip, it spits out a list of indices which refer directly to the
// vertices in the MDL file.
std::vector<SourceVtxDataAccessor::IndexList> SourceVtxDataAccessor::computeMdlVertexIndices(
  size_t bodyPartIndex, size_t submodelIndex, size_t lodIndex, size_t meshIndex) {
  if (bodyPartIndex >= static_cast<size_t>(m_header.numBodyParts)) {
    throw AssetException(
      "Body part index " + std::to_string(bodyPartIndex) + " exceeded number of body parts");
  }

  SourceVtxLayout::BodyPart bodyPart{};
  const size_t bodyPartFileOffset =
    extractItem(static_cast<size_t>(m_header.bodyPartOffset), bodyPartIndex, bodyPart);

  if (submodelIndex >= static_cast<size_t>(bodyPart.numModels)) {
    throw AssetException(
      "Submodel index " + std::to_string(submodelIndex) + " exceeded number of submodels");
  }

  return computeMdlVertexIndices_SubModel(
    bodyPartFileOffset + static_cast<size_t>(bodyPart.modelOffset), submodelIndex, lodIndex,
    meshIndex);
}

std::vector<SourceVtxDataAccessor::IndexList> SourceVtxDataAccessor::
  computeMdlVertexIndices_SubModel(
    size_t submodelFileOffset, size_t submodelIndex, size_t lodIndex, size_t meshIndex) {
  SourceVtxLayout::Model submodel{};
  const size_t submodelOffset = extractItem(submodelFileOffset, submodelIndex, submodel);

  if (lodIndex >= static_cast<size_t>(submodel.numLODs)) {
    throw AssetException("LOD index " + std::to_string(lodIndex) + " exceeded number of LODs");
  }

  return computeMdlVertexIndices_LOD(
    submodelOffset + static_cast<size_t>(submodel.lodOffset), lodIndex, meshIndex);
}

std::vector<SourceVtxDataAccessor::IndexList> SourceVtxDataAccessor::computeMdlVertexIndices_LOD(
  size_t lodFileOffset, size_t lodIndex, size_t meshIndex) {
  SourceVtxLayout::LOD lod{};
  const size_t lodOffset = extractItem(lodFileOffset, lodIndex, lod);

  if (meshIndex >= static_cast<size_t>(lod.numMeshes)) {
    throw AssetException("Mesh index " + std::to_string(meshIndex) + " exceeded number of meshes");
  }

  return computeMdlVertexIndices_Mesh(lodOffset + static_cast<size_t>(lod.meshOffset), meshIndex);
}

std::vector<SourceVtxDataAccessor::IndexList> SourceVtxDataAccessor::computeMdlVertexIndices_Mesh(
  size_t meshFileOffset, size_t meshIndex) {
  SourceVtxLayout::Mesh mesh{};
  const size_t meshOffset = extractItem(meshFileOffset, meshIndex, mesh);

  std::vector<IndexList> outList;
  computeIndicesForMesh(meshOffset, mesh, outList);
  return outList;
}

void SourceVtxDataAccessor::computeIndicesForMesh(
  size_t meshOffset, const SourceVtxLayout::Mesh& mesh, std::vector<IndexList>& out) {
  std::vector<SourceVtxLayout::StripGroup> stripGroups = extractItems<SourceVtxLayout::StripGroup>(
    meshOffset + static_cast<size_t>(mesh.stripGroupHeaderOffset),
    static_cast<size_t>(mesh.numStripGroups));

  for (size_t index = 0; index < stripGroups.size(); ++index) {
    const size_t stripGroupOffset = meshOffset + static_cast<size_t>(mesh.stripGroupHeaderOffset) +
                                    (index * sizeof(SourceVtxLayout::StripGroup));
    computeIndicesForStripGroup(stripGroupOffset, stripGroups[index], out);
  }
}

void SourceVtxDataAccessor::computeIndicesForStripGroup(
  size_t stripGroupOffset, const SourceVtxLayout::StripGroup& group, std::vector<IndexList>& out) {
  std::vector<SourceVtxLayout::Vertex> vertices = extractItems<SourceVtxLayout::Vertex>(
    stripGroupOffset + static_cast<size_t>(group.vertOffset), static_cast<size_t>(group.numVerts));

  // These index into the vertex buffer above
  std::vector<SourceVtxLayout::Index> indices = extractItems<SourceVtxLayout::Index>(
    stripGroupOffset + static_cast<size_t>(group.indexOffset),
    static_cast<size_t>(group.numIndices));

  std::vector<SourceVtxLayout::Strip> strips = extractItems<SourceVtxLayout::Strip>(
    stripGroupOffset + static_cast<size_t>(group.stripOffset),
    static_cast<size_t>(group.numStrips));

  for (const SourceVtxLayout::Strip& strip : strips) {
    computeIndicesForStrip(strip, vertices, indices, out);
  }
}

void SourceVtxDataAccessor::computeIndicesForStrip(
  const SourceVtxLayout::Strip& strip, const std::vector<SourceVtxLayout::Vertex>& vertices,
  const std::vector<SourceVtxLayout::Index>& indices, std::vector<IndexList>& out) {
  const bool verticesExceedBuffer =
    static_cast<size_t>(strip.vertOffset) + static_cast<size_t>(strip.numVerts) > vertices.size();
  const bool noVertices = strip.numVerts < 1;
  const bool indicesExceedBuffer =
    static_cast<size_t>(strip.indexOffset) + static_cast<size_t>(strip.numIndices) > indices.size();
  const bool noIndices = strip.numIndices < 1;

  if (verticesExceedBuffer || noVertices || indicesExceedBuffer || noIndices) {
    return;
  }

  if (
    !(strip.flags & SourceVtxLayout::STRIP_IS_TRILIST) &&
    !(strip.flags & SourceVtxLayout::STRIP_IS_TRISTRIP)) {
    // If neither of these flags are present, the strip is not valid, so just ignore.
    return;
  }

  computeIndices(
    vertices, indices, static_cast<size_t>(strip.indexOffset),
    static_cast<size_t>(strip.numIndices), strip.flags & SourceVtxLayout::STRIP_IS_TRISTRIP, out);
}

void SourceVtxDataAccessor::computeIndices(
  const std::vector<SourceVtxLayout::Vertex>& vertices,
  const std::vector<SourceVtxLayout::Index>& indices, size_t iOffset, size_t iCount,
  bool isTriStrip, std::vector<IndexList>& out) {
  if (!isTriStrip && iCount % 3 != 0) {
    throw AssetException(
      "Expected indices to be a multiple of 3 for a tri list, but got " + std::to_string(iCount) +
      " indices");
  }

  IndexList& outIndexList = out.emplace_back();
  outIndexList.isTriStrip = isTriStrip;
  outIndexList.indices.reserve(iCount);

  for (size_t localIndex = 0; localIndex < iCount; ++localIndex) {
    const SourceVtxLayout::Index& stripIndex = indices[iOffset + localIndex];
    const size_t vIndex = stripIndex.value;

    if (vIndex >= vertices.size()) {
      throw AssetException(
        "Encountered strip index value " + std::to_string(vIndex) + " that exceeds buffer of " +
        std::to_string(vertices.size()) + " vertices");
    }

    const SourceVtxLayout::Vertex& stripVertex = vertices[vIndex];
    outIndexList.indices.emplace_back(stripVertex.origMeshVertID);
  }
}
} // namespace IO
} // namespace TrenchBroom
