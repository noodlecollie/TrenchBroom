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

#include "IO/VtfHeaderBuffer.h"
#include "Ensure.h"
#include "Exceptions.h"

namespace TrenchBroom {
namespace IO {
template <typename T>
static void ReadIntoBuffer(TrenchBroom::IO::Reader& reader, std::vector<uint8_t>& buffer) {
  buffer.resize(sizeof(T));

  reader.seekFromBegin(0);
  reader.read(buffer.data(), buffer.size());
}

VtfHeaderBuffer::VtfHeaderBuffer(Reader&& inReader) {
  Vtf::HeaderBase headerBase;

  Reader reader = std::move(inReader);
  reader.read(reinterpret_cast<char*>(&headerBase), sizeof(headerBase));

  if (headerBase.version[0] > 7 || (headerBase.version[0] == 7 && headerBase.version[1] > 5)) {
    throw FileFormatException("VTF header version was newer than newest supported version 7.5");
  }

  if (headerBase.version[0] < 7) {
    throw FileFormatException("VTF header version was older than oldest supported version 7.0");
  }

  switch (headerBase.version[1]) {
    case 5:
    case 4:
    case 3: {
      ReadIntoBuffer<Vtf::Header_73_A>(reader, m_buffer);
      readResources(reader);
      break;
    }

    case 2: {
      ReadIntoBuffer<Vtf::Header_72_A>(reader, m_buffer);
      break;
    }

    case 1:
    case 0: {
      ReadIntoBuffer<Vtf::Header_70_A>(reader, m_buffer);
      break;
    }

    default: {
      ensure(false, "Unhandled VTF minor version number, should never happen");
    }
  }
}

uint32_t VtfHeaderBuffer::majorVersion() const {
  return headerBase()->version[0];
}

uint32_t VtfHeaderBuffer::minorVersion() const {
  return headerBase()->version[1];
}

bool VtfHeaderBuffer::versionIsAtLeast(uint32_t major, uint32_t minor) const {
  const Vtf::HeaderBase* header = headerBase();

  return header->version[0] > major || (header->version[0] == major && header->version[1] >= minor);
}

size_t VtfHeaderBuffer::resourceCount() const {
  return m_resources.size();
}
const Vtf::Resource* VtfHeaderBuffer::resource(size_t index) const {
  return index < m_resources.size() ? (m_resources.data() + index) : nullptr;
}

bool VtfHeaderBuffer::findResourceData(uint32_t resourceType, uint32_t& outData) const {
  for (const Vtf::Resource& resource : m_resources) {
    if (resource.type == resourceType) {
      outData = resource.data;
      return true;
    }
  }

  return false;
}

const Vtf::HeaderBase* VtfHeaderBuffer::headerBase() const {
  const Vtf::HeaderBase* header = castBuffer<Vtf::HeaderBase>();
  ensure(header, "Header data was not valid");
  return header;
}

void VtfHeaderBuffer::readResources(Reader& reader) {
  const Vtf::Header_73_A* header73 = header<Vtf::Header_73_A>();
  ensure(header73, "Header data was not valid for v7.3+ VTF file");

  if (header73->resourceCount < 1) {
    return;
  }

  // Note that the headerSize member is the size of the enture header including all resources!
  // We only want to wind on to the end of the header struct itself, to find the resources.
  reader.seekFromBegin(sizeof(Vtf::Header_73_A));
  m_resources.resize(header73->resourceCount);

  for (uint32_t index = 0; index < header73->resourceCount; ++index) {
    reader.read(reinterpret_cast<char*>(m_resources.data() + index), sizeof(Vtf::Resource));
  }
}
} // namespace IO
} // namespace TrenchBroom
