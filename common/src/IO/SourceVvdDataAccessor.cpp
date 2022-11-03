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

#include "IO/SourceVvdDataAccessor.h"
#include "Exceptions.h"
#include "IO/File.h"
#include "IO/ReaderException.h"
#include "IO/SourceMdlFormatUtils.h"
#include "IO/SourceMdlHelpers.h"
#include "IO/SourceVvdLayout.h"
#include <string>

namespace TrenchBroom {
namespace IO {
SourceVvdDataAccessor::SourceVvdDataAccessor(const std::shared_ptr<File>& file)
  : m_file(file)
  , m_reader(m_file->reader().buffer()) {
  m_reader.read(reinterpret_cast<char*>(&m_header), sizeof(m_header));
}

void SourceVvdDataAccessor::validate(SourceMdlLayout::Header mdlHeader) const {
  if (!isSourceVvdIdentifier(m_header.id)) {
    throw FileFormatException("Unknown Source VVD ident: " + std::to_string(m_header.id));
  }

  if (!isSourceVvdVersion(m_header.version)) {
    throw FileFormatException(
      "Unsupported Source VVD version: " + std::to_string(m_header.version));
  }
  if (m_header.checksum != mdlHeader.checksum) {
    throw FileFormatException(
      "Source VVD file checksum " + std::to_string(m_header.checksum) +
      " did not match MDL file checksum " + std::to_string(mdlHeader.checksum));
  }

  // Should never happen:
  if (static_cast<size_t>(m_header.numLODs) > ArraySize(m_header.numLODVertexes)) {
    throw FileFormatException(
      "LOD count of " + std::to_string(m_header.numLODs) + " exceeded size of lod vertices array");
  }
}

// The whole vertex fix-up thing was really confusing. This process is based off the
// Studio_LoadVertexes function in the engine, which was the best example I could find of how vertex
// fix-ups and LODs are supposed to work together.
const std::vector<SourceVvdLayout::Vertex>& SourceVvdDataAccessor::consolidateVertices(
  size_t rootLOD) {
  if (rootLOD >= static_cast<size_t>(m_header.numLODs)) {
    throw ReaderException(
      "Root LOD value of " + std::to_string(rootLOD) + " was out of range for VVD file");
  }

  if (rootLOD == m_rootLOD && !m_vertices.empty()) {
    // Already consolidated.
    return m_vertices;
  }

  m_rootLOD = rootLOD;
  m_vertices.clear();

  if (m_header.numFixups > 0) {
    consolidateVerticesWithFixUp();
  } else {
    consolidatePlainVertices();
  }

  return m_vertices;
}

void SourceVvdDataAccessor::consolidatePlainVertices() {
  m_vertices.resize(static_cast<size_t>(m_header.numLODVertexes[m_rootLOD]));

  m_reader.seekFromBegin(static_cast<size_t>(m_header.vertexDataStart));
  m_reader.read(
    reinterpret_cast<char*>(m_vertices.data()),
    m_vertices.size() * sizeof(SourceVvdLayout::Vertex));
}

void SourceVvdDataAccessor::consolidateVerticesWithFixUp() {
  std::vector<SourceVvdLayout::VertexFixUp> fixUps;
  fixUps.resize(static_cast<size_t>(m_header.numFixups));

  m_reader.seekFromBegin(static_cast<size_t>(m_header.fixupTableStart));
  m_reader.read(
    reinterpret_cast<char*>(fixUps.data()), fixUps.size() * sizeof(SourceVvdLayout::VertexFixUp));

  size_t numVertices = 0;

  // Do a quick first pass to pre-compute the vertex array size.
  // This is probably more efficient than resizing a potentially large buffer multiple times.
  for (const SourceVvdLayout::VertexFixUp& fixUp : fixUps) {
    if (static_cast<size_t>(fixUp.lod) < m_rootLOD) {
      continue;
    }

    numVertices += static_cast<size_t>(fixUp.numVertices);
  }

  m_vertices.reserve(numVertices);

  // Now do a second pass for copying.
  for (const SourceVvdLayout::VertexFixUp& fixUp : fixUps) {
    if (static_cast<size_t>(fixUp.lod) < m_rootLOD) {
      continue;
    }

    const size_t base = m_vertices.size();
    const size_t verticesToRead = static_cast<size_t>(fixUp.numVertices);
    m_vertices.resize(base + verticesToRead);

    m_reader.seekFromBegin(
      static_cast<size_t>(m_header.vertexDataStart) +
      (static_cast<size_t>(fixUp.sourceVertexID) * sizeof(SourceVvdLayout::Vertex)));

    m_reader.read(
      reinterpret_cast<char*>(m_vertices.data()) + (base * sizeof(SourceVvdLayout::Vertex)),
      verticesToRead * sizeof(SourceVvdLayout::Vertex));
  }
}
} // namespace IO
} // namespace TrenchBroom
