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

#include "IO/Reader.h"
#include "IO/SourceMdlLayout.h"
#include "IO/SourceVtxLayout.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace TrenchBroom {
namespace IO {
class File;

class SourceVtxDataAccessor {
public:
  struct IndexList {
    // True = tri stip, false = tri list
    bool isTriStrip;
    std::vector<uint16_t> indices;
  };

  // Assumes file is valid
  explicit SourceVtxDataAccessor(const std::shared_ptr<File>& file);

  void validate(SourceMdlLayout::Header mdlHeader) const;
  std::vector<IndexList> computeMdlVertexIndices(
    size_t bodyPartIndex, size_t submodelIndex, size_t lodIndex, size_t meshIndex);

private:
  template <typename T> inline std::vector<T> extractItems(size_t offset, size_t count) {
    std::vector<T> items;

    items.resize(count);

    m_reader.seekFromBegin(offset);
    m_reader.read(reinterpret_cast<char*>(items.data()), items.size() * sizeof(T));

    return items;
  }

  template <typename T> size_t extractItem(size_t base, size_t index, T& out) {
    const size_t offset = base + (index * sizeof(T));
    m_reader.seekFromBegin(offset);
    m_reader.read(reinterpret_cast<char*>(&out), sizeof(T));
    return offset;
  }

  std::vector<IndexList> computeMdlVertexIndices_SubModel(
    size_t submodelFileOffset, size_t submodelIndex, size_t lodIndex, size_t meshIndex);
  std::vector<IndexList> computeMdlVertexIndices_LOD(
    size_t lodFileOffset, size_t lodIndex, size_t meshIndex);
  std::vector<IndexList> computeMdlVertexIndices_Mesh(size_t meshFileOffset, size_t meshIndex);
  void computeIndicesForMesh(
    size_t meshOffset, const SourceVtxLayout::Mesh& mesh, std::vector<IndexList>& out);
  void computeIndicesForStripGroup(
    size_t stripGroupOffset, const SourceVtxLayout::StripGroup& group, std::vector<IndexList>& out);
  void computeIndicesForStrip(
    const SourceVtxLayout::Strip& strip, const std::vector<SourceVtxLayout::Vertex>& vertices,
    const std::vector<SourceVtxLayout::Index>& indices, std::vector<IndexList>& out);
  void computeIndices(
    const std::vector<SourceVtxLayout::Vertex>& vertices,
    const std::vector<SourceVtxLayout::Index>& indices, size_t iOffset, size_t iCount,
    bool isTriStrip, std::vector<IndexList>& out);

  std::shared_ptr<File> m_file;
  BufferedReader m_reader;
  SourceVtxLayout::Header m_header;
};
} // namespace IO
} // namespace TrenchBroom
