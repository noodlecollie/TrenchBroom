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

#include "IO/SourceMdlFormatUtils.h"
#include "IO/Reader.h"

namespace TrenchBroom {
namespace IO {

#pragma pack(push, 1)
struct MdlVersionHeader {
  int32_t id;
  int32_t version;
};
#pragma pack(pop)

int32_t getMdlVersion(Reader& reader) {
  const size_t origPos = reader.position();
  reader.seekFromBegin(0);

  MdlVersionHeader header{};
  reader.read(reinterpret_cast<char*>(&header), sizeof(header));
  reader.seekFromBegin(origPos);

  return header.version;
}

bool isSourceMdlIdentifier(int32_t ident) {
  static constexpr int32_t IDENT = (('T' << 24) + ('S' << 16) + ('D' << 8) + 'I');
  return ident == IDENT;
}

// Unsure if there's a list of known versions somewhere
// (Google has not been as helpful as I'd hoped).
// For now, we explicitly whitelist versions
bool isSourceMdlVersion(int32_t version) {
  switch (version) {
    case 44:
    case 45:
    case 48: {
      return true;
    }

    default: {
      return false;
    }
  }
}

bool isSourceVvdIdentifier(int32_t ident) {
  static constexpr int32_t IDENT = (('V' << 24) + ('S' << 16) + ('D' << 8) + 'I');
  return ident == IDENT;
}

bool isSourceVvdVersion(int32_t version) {
  switch (version) {
    case 4: {
      return true;
    }

    default: {
      return false;
    }
  }
}

bool isSourceVtxVersion(int32_t version) {
  switch (version) {
    case 7: {
      return true;
    }

    default: {
      return false;
    }
  }
}
} // namespace IO
} // namespace TrenchBroom
