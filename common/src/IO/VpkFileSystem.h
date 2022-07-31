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

#include "IO/ImageFileSystem.h"

#include <memory>
#include <utility>

namespace TrenchBroom {
class Logger;

namespace IO {
class Reader;

class VpkFileSystem : public ImageFileSystemBase {
public:
  explicit VpkFileSystem(const Path& path, Logger& logger);
  VpkFileSystem(std::shared_ptr<FileSystem> next, const Path& path, Logger& logger);

private:
  struct DirEntry;
  struct OffsetLengthPair {
    size_t offset = 0;
    size_t length = 0;
  };

  void doReadDirectory() override;

  static std::string readNTString(Reader& reader);
  static void readDirEntry(Reader& dirTreeReader, const Path& path, DirEntry& entry);

  void enumerateAllVpkFiles(const Path& path);
  void readAndParseDirectoryListing(Reader& dirTreeReader, const OffsetLengthPair& dirFileData);
  void addFile(const Path& path, const DirEntry& entry, const OffsetLengthPair& dirFileData);
  void addFile(
    const Path& path, const std::shared_ptr<CFile>& archive, const OffsetLengthPair& fileExtent,
    OffsetLengthPair archiveExtent = OffsetLengthPair{});

  Logger& m_logger;
  std::shared_ptr<CFile> m_dirFile;
  std::vector<std::shared_ptr<CFile>> m_auxFiles;
};
} // namespace IO
} // namespace TrenchBroom
