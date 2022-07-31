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

#include "IO/VpkUtils.h"
#include "kdl/string_compare.h"
#include "kdl/string_utils.h"

namespace TrenchBroom {
namespace IO {
namespace Vpk {
bool isVpk(const Path& path) {
  return kdl::ci::str_is_equal(path.extension(), "vpk");
}

bool isArchiveDir(const Path& path) {
  return kdl::ci::str_is_suffix(path.basename(), "_dir");
}
std::string getArchiveName(const Path& path) {
  const std::string baseName = path.basename();
  const size_t underscoreIndex = baseName.rfind('_');

  if (underscoreIndex == std::string::npos) {
    // No underscore, so the whole base name indicates the archive name.
    return baseName;
  }

  const std::string suffix = baseName.substr(underscoreIndex + 1);

  // Is the suffix "dir" or a number?
  if (kdl::ci::str_is_equal(suffix, "dir") || kdl::str_to_int(suffix).has_value()) {
    // The archive name is everything up to the underscore.
    return baseName.substr(0, underscoreIndex);
  }

  // There was an underscore but the suffix did not correspond to a known format, so treat the whole
  // base name as the archive name.
  return baseName;
}

std::optional<int> getArchiveIndex(const Path& path) {
  const std::string baseName = path.basename();
  const size_t underscoreIndex = baseName.rfind('_');

  if (underscoreIndex == std::string::npos) {
    return std::optional<int>();
  }

  return kdl::str_to_int(baseName.substr(underscoreIndex + 1));
}

std::vector<Path> removeIndexedArchivesFromList(const std::vector<Path>& list) {
  std::vector<Path> dirs;
  std::vector<Path> output = list;

  for (const Path& entry : list) {
    if (isVpk(entry) && isArchiveDir(entry)) {
      dirs.emplace_back(entry);
    }
  }

  for (const Path& dirEntry : dirs) {
    const std::string archiveName = getArchiveName(dirEntry);

    auto it = output.begin();
    while (it != output.end()) {
      if (isVpk(*it) && getArchiveName(*it) == archiveName && !isArchiveDir(*it)) {
        it = output.erase(it);
      } else {
        ++it;
      }
    }
  }

  return output;
}
} // namespace Vpk
} // namespace IO
} // namespace TrenchBroom
