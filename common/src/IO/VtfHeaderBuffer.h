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

#include "IO/File.h"
#include "IO/Reader.h"
#include "IO/VtfDefs.h"
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

namespace TrenchBroom {
namespace IO {
class VtfHeaderBuffer {
public:
  explicit VtfHeaderBuffer(Reader&& inReader);

  uint32_t majorVersion() const;
  uint32_t minorVersion() const;

  bool versionIsAtLeast(uint32_t major, uint32_t minor) const;

  // Guaranteed to always return a valid pointer for Vtf::Header_70,
  // since the creation of the class would have failed otherwise.
  template <typename T>
  typename std::enable_if<std::is_base_of<Vtf::Header_70, T>::value, const T*>::type header()
    const {
    return castBuffer<T>();
  }

  const Vtf::Header_70* basicHeader() const { return header<Vtf::Header_70>(); }

  size_t resourceCount() const;
  const Vtf::Resource* resource(size_t index) const;
  bool findResourceData(uint32_t resourceType, uint32_t& outData) const;

private:
  template <typename T> const T* castBuffer() const {
    return m_buffer.size() >= sizeof(T) ? reinterpret_cast<const T*>(m_buffer.data()) : nullptr;
  }

  const Vtf::HeaderBase* headerBase() const;
  void readResources(Reader& reader);

  std::vector<uint8_t> m_buffer;
  std::vector<Vtf::Resource> m_resources;
};
} // namespace IO
} // namespace TrenchBroom
