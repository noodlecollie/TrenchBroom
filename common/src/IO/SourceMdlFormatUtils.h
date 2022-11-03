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
class Reader;

int32_t getMdlVersion(Reader& reader);
bool isSourceMdlIdentifier(int32_t ident);
bool isSourceMdlVersion(int32_t version);

bool isSourceVvdIdentifier(int32_t ident);
bool isSourceVvdVersion(int32_t version);

bool isSourceVtxVersion(int32_t version);
} // namespace IO
} // namespace TrenchBroom
