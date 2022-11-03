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
#include "IO/SourceVvdLayout.h"
#include <memory>
#include <vector>

namespace TrenchBroom {
namespace IO {
class File;

class SourceVvdDataAccessor {
public:
  // Assumes file is valid
  explicit SourceVvdDataAccessor(const std::shared_ptr<File>& file);

  void validate(SourceMdlLayout::Header mdlHeader) const;
  const std::vector<SourceVvdLayout::Vertex>& consolidateVertices(size_t rootLOD);

private:
  void consolidatePlainVertices();
  void consolidateVerticesWithFixUp();

  std::shared_ptr<File> m_file;
  BufferedReader m_reader;
  SourceVvdLayout::Header m_header;
  std::vector<SourceVvdLayout::Vertex> m_vertices;
  size_t m_rootLOD = 0;
};
} // namespace IO
} // namespace TrenchBroom
