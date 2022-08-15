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

#include "VpkFileSystem.h"

#include <cstdint>

#include "Ensure.h"
#include "IO/DiskIO.h"
#include "IO/File.h"
#include "IO/FileMatcher.h"
#include "IO/VpkUtils.h"
#include "Logger.h"
#include "kdl/string_compare.h"
#include "kdl/string_utils.h"

namespace TrenchBroom {
namespace IO {
// Reasonable maximum for multi-part VPKs.
constexpr size_t MAX_VPK_PARTS = 200;

constexpr uint32_t VPKV2_SIGNATURE = 0x55aa1234;
constexpr uint32_t VPKV2_VERSION = 2;
constexpr uint16_t VPK2_DIR_ARCHIVE_INDEX = 0x7fff;

#pragma pack(push, 1)
struct VPKV2_Header {
  // Should always be 0x55aa1234.
  uint32_t signature;

  // Should always be 2.
  uint32_t version;

  // The size, in bytes, of the directory tree
  uint32_t treeSize;

  // How many bytes of file content are stored in this VPK file (0 in CSGO)
  uint32_t fileDataSectionSize;

  // The size, in bytes, of the section containing MD5 checksums for external archive content
  uint32_t archiveMD5SectionSize;

  // The size, in bytes, of the section containing MD5 checksums for content in this file (should
  // always be 48)
  uint32_t otherMD5SectionSize;

  // The size, in bytes, of the section containing the public key and signature. This is either 0
  // (CSGO & The Ship) or 296 (HL2, HL2:DM, HL2:EP1, HL2:EP2, HL2:LC, TF2, DOD:S & CS:S)
  uint32_t signatureSectionSize;
};

struct VPKV2_DirEntry {
  // A 32-bit CRC of the file's data.
  uint32_t crc;

  // The number of bytes contained in the index file.
  uint16_t preloadBytes;

  // A zero based index of the archive this file's data is contained in.
  // If 0x7fff, the data follows the directory.
  uint16_t archiveIndex;

  // If ArchiveIndex is 0x7fff, the offset of the file data relative to the end of the directory
  // (see the header for more details). Otherwise, the offset of the data from the start of the
  // specified archive.
  uint32_t entryOffset;

  // If zero, the entire file is stored in the preload data.
  // Otherwise, the number of bytes stored starting at EntryOffset.
  uint32_t entryLength;

  // Should always be 0xffff.
  uint16_t terminator;
};
#pragma pack(pop)

// We don't want to bother about listing files in VPKs that don't fall under these extensions:
static const std::vector<std::string> SUPPORTED_EXTENSIONS = {
  "vmt",
  "vtf",
  "mdl",
};

struct VpkFileSystem::DirEntry {
  VPKV2_DirEntry rawEntry;
  std::vector<uint8_t> preloadData;
};

VpkFileSystem::VpkFileSystem(const Path& path, Logger& logger)
  : VpkFileSystem(nullptr, path, logger) {}

VpkFileSystem::VpkFileSystem(std::shared_ptr<FileSystem> next, const Path& path, Logger& logger)
  : ImageFileSystemBase(std::move(next), path)
  , m_logger(logger) {
  ensure(m_path.isAbsolute(), "path must be absolute");

  enumerateAllVpkFiles(path);
  initialize();
}

void VpkFileSystem::enumerateAllVpkFiles(const Path& path) {
  m_dirFile.reset();
  m_auxFiles.clear();

  if (!Vpk::isArchiveDir(path)) {
    throw FileSystemException(path.filename() + " was not a VPK directory file");
  }

  m_dirFile = std::make_shared<CFile>(path);

  const std::string archiveName = Vpk::getArchiveName(path);
  const Path directory = path.deleteLastComponent();
  const std::vector<Path> vpkFiles =
    Disk::findItems(directory, FileNameMatcher(archiveName + "_*.vpk"));

  for (const auto& entry : vpkFiles) {
    if (Vpk::isArchiveDir(entry)) {
      // We already dealt with this.
      continue;
    }

    const std::optional<int> index = Vpk::getArchiveIndex(entry);

    if (!index.has_value() || *index < 0) {
      // Just ignore this file.
      continue;
    }

    const size_t unsignedIndex = static_cast<size_t>(*index);

    if (unsignedIndex > MAX_VPK_PARTS) {
      throw FileSystemException(
        entry.filename() + "exceeded max supported index " + std::to_string(MAX_VPK_PARTS));
    }

    // No guarantee (as far as I know) that the files will be listed in ascending order with
    // respect to the indices, so always make sure the list is long enough to accommodate this
    // particular index.
    if (unsignedIndex >= m_auxFiles.size()) {
      m_auxFiles.resize(unsignedIndex + 1);
    }

    m_auxFiles[unsignedIndex] = std::make_shared<CFile>(entry);
  }

  // If we didn't have a dir file, we shouldn't have got this far.
  ensure(m_dirFile, "No directory VPK found");

  // See if there are any holes in the list of VPKs, indicating a missing part.
  // All VPK parts should be consecutive and begin at 0.
  for (size_t index = 0; index < m_auxFiles.size(); ++index) {
    if (!m_auxFiles[index]) {
      throw FileSystemException("Missing " + archiveName + "_" + std::to_string(index) + ".vpk");
    }
  }
}

void VpkFileSystem::doReadDirectory() {
  if (!m_dirFile) {
    throw FileSystemException("VPK directory file was not initialised");
  }

  auto reader = m_dirFile->reader();

  if (reader.size() < sizeof(VPKV2_Header)) {
    throw FileSystemException("VPK directory file is not valid");
  }

  auto header = reader.read<VPKV2_Header, VPKV2_Header>();

  if (header.signature != VPKV2_SIGNATURE) {
    throw FileSystemException("VPK file signature was invalid");
  }

  if (header.version != VPKV2_VERSION) {
    throw FileSystemException(
      "Expected version " + std::to_string(VPKV2_VERSION) +
      " directory VPK file, but got version " + std::to_string(header.version));
  }

  if (header.treeSize < 1) {
    throw FileSystemException("VPK directory tree size was zero");
  }

  auto dirTreeReader = reader.subReaderFromCurrent(header.treeSize);
  const size_t fileDataOffset = reader.position() + header.treeSize;
  readAndParseDirectoryListing(
    dirTreeReader, OffsetLengthPair{fileDataOffset, header.fileDataSectionSize});
}

Path VpkFileSystem::doMakeAbsolute(const Path& path) const {
  return m_path.deleteLastComponent() + path;
}

void VpkFileSystem::readAndParseDirectoryListing(
  Reader& dirTreeReader, const OffsetLengthPair& dirFileData) {
  while (true) {
    const std::string extension = readNTString(dirTreeReader);

    if (extension.empty()) {
      break;
    }

    const bool skipFile =
      std::find(SUPPORTED_EXTENSIONS.begin(), SUPPORTED_EXTENSIONS.end(), extension) ==
      SUPPORTED_EXTENSIONS.end();

    while (true) {
      const std::string path = readNTString(dirTreeReader);

      if (path.empty()) {
        break;
      }

      while (true) {
        const std::string fileName = readNTString(dirTreeReader);

        if (fileName.empty()) {
          break;
        }

        const Path filePath(path + "/" + fileName + "." + extension);
        DirEntry entry{};
        readDirEntry(dirTreeReader, filePath, entry);

        if (!skipFile) {
          addFile(filePath, entry, dirFileData);
        }
      }
    }
  }
}

void VpkFileSystem::readDirEntry(Reader& dirTreeReader, const Path& path, DirEntry& entry) {
  entry.rawEntry = dirTreeReader.read<VPKV2_DirEntry, VPKV2_DirEntry>();

  if (entry.rawEntry.preloadBytes > 0) {
    if (!dirTreeReader.canRead(entry.rawEntry.preloadBytes)) {
      throw FileSystemException("Corrupt preload bytes when parsing " + path.asString());
    }

    entry.preloadData.resize(entry.rawEntry.preloadBytes);
    dirTreeReader.read(entry.preloadData.data(), entry.preloadData.size());
  }
}

void VpkFileSystem::addFile(
  const Path& path, const DirEntry& entry, const OffsetLengthPair& dirFileData) {
  if (
    entry.rawEntry.archiveIndex != VPK2_DIR_ARCHIVE_INDEX &&
    static_cast<size_t>(entry.rawEntry.archiveIndex) >= m_auxFiles.size()) {
    m_logger.warn(
      "File " + path.asString() + " was located in invalid VPK archive " +
      std::to_string(entry.rawEntry.archiveIndex) + ", skipping");
    return;
  }

  if (entry.preloadData.size() > 0) {
    // This would mean that not all data is located in the target aux archive.
    // Not currently supported, but we could add support for this in future.
    // It would mean copying the rest of the data from the aux archive and concatenating
    // it with the preload data, then holding the entire file in memory.
    m_logger.warn(
      "File " + path.asString() + " contained preload data, which is not currently supported.");
    return;
  }

  const OffsetLengthPair extent{entry.rawEntry.entryOffset, entry.rawEntry.entryLength};

  if (extent.length < 1) {
    m_logger.warn("File " + path.asString() + " had no data, skipping.");
    return;
  }

  if (entry.rawEntry.archiveIndex == VPK2_DIR_ARCHIVE_INDEX) {
    addFile(path, m_dirFile, extent, dirFileData);
  } else {
    addFile(path, m_auxFiles[entry.rawEntry.archiveIndex], extent);
  }
}

void VpkFileSystem::addFile(
  const Path& path, const std::shared_ptr<CFile>& archive, const OffsetLengthPair& fileExtent,
  OffsetLengthPair archiveExtent) {
  if (archiveExtent.offset == 0 && archiveExtent.length == 0) {
    archiveExtent.length = archive->size();
  }

  if (
    fileExtent.offset < archiveExtent.offset ||
    fileExtent.offset + fileExtent.length > archiveExtent.offset + archiveExtent.length) {
    m_logger.warn(
      "File " + path.asString() + " was out of range of data within VPK archive, skipping.");
    return;
  }

  auto file = std::make_shared<FileView>(path, archive, fileExtent.offset, fileExtent.length);
  m_root.addFile(path, file);
}

std::string VpkFileSystem::readNTString(Reader& reader) {
  const size_t begin = reader.position();
  while (reader.readChar<char>() != '\0') {
    // Keep reading until the terminator is reached.
  }

  const size_t length = reader.position() - begin - 1;
  reader.seekFromBegin(begin);
  const std::string out = reader.readString(length);

  // Skip over null terminator.
  reader.readChar<char>();

  return out;
}

} // namespace IO
} // namespace TrenchBroom
