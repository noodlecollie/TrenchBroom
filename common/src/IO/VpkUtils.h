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

#include "IO/Path.h"
#include <optional>
#include <string>

namespace TrenchBroom {
namespace IO {
namespace Vpk {
bool isVpk(const Path& path);
bool isArchiveDir(const Path& path);
std::string getArchiveName(const Path& path);
std::optional<int> getArchiveIndex(const Path& path);

// For any archives that are identified as a directory archive, removes all of the corresponding
// indexed archive parts. This is because these indexed parts are not "filesystems" in themselves,
// they are just used by the directory.
std::vector<Path> removeIndexedArchivesFromList(const std::vector<Path>& list);
} // namespace Vpk
} // namespace IO
} // namespace TrenchBroom
