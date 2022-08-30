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

#include "IO/VmtTextureReader.h"
#include "Assets/Texture.h"
#include "Ensure.h"
#include "IO/File.h"
#include "IO/FileSystem.h"
#include "IO/VtfDefs.h"
#include "IO/VtfHeaderBuffer.h"
#include "IO/VtfUtils.h"
#include "Logger.h"
#include <cstdint>
#include <cstring>

namespace TrenchBroom {
namespace IO {
static std::vector<char>::const_iterator findStringCaseInsensitive(
  const std::vector<char>& strHaystack, const std::string& strNeedle,
  std::vector<char>::difference_type offset = 0) {
  const std::vector<char>::const_iterator begin = strHaystack.begin() + offset;
  return std::search(
    begin, strHaystack.end(), strNeedle.begin(), strNeedle.end(), [](char ch1, char ch2) {
      return std::toupper(ch1) == std::toupper(ch2);
    });
}

// Super dumb function just finds the first instance of "$basetexture"
static std::string getBaseTextureFromKeyValues(Reader& reader) {
  static constexpr char BASETEXTURE_KEY[] = "$basetexture";

  std::vector<char> buffer(reader.size() + 1);

  reader.seekFromBegin(0);
  reader.read(buffer.data(), buffer.size() - 1);
  buffer[buffer.size() - 1] = '\0';

  std::vector<char>::difference_type offset = 0;

  while (true) {
    const std::vector<char>::const_iterator baseTextureKey =
      findStringCaseInsensitive(buffer, BASETEXTURE_KEY, offset);

    if (baseTextureKey == buffer.end()) {
      break;
    }

    // Find the beginning of the value.
    std::vector<char>::const_iterator valueBegin = baseTextureKey + sizeof(BASETEXTURE_KEY) - 1;
    const bool keyEndWasValid = *valueBegin == '"' || *valueBegin == ' ' || *valueBegin == '\t';

    if (!keyEndWasValid) {
      offset = valueBegin - buffer.begin();
      continue;
    }

    if (*valueBegin == '"') {
      ++valueBegin;
    }

    while (*valueBegin && std::isspace(*valueBegin)) {
      ++valueBegin;
    }

    if (!(*valueBegin)) {
      break;
    }

    std::vector<char>::const_iterator valueEnd = valueBegin;

    while (*valueEnd && *valueEnd != '\r' && *valueEnd != '\n') {
      ++valueEnd;
    }

    // Trim whitespace
    while (valueEnd > valueBegin && std::isspace(*(valueEnd - 1))) {
      --valueEnd;
    }

    // Trim quotes
    if (valueEnd - valueBegin >= 2 && *valueBegin == '"' && *(valueEnd - 1) == '"') {
      ++valueBegin;
      --valueEnd;
    }

    return std::string(valueBegin, valueEnd);
  }

  return std::string();
}

static size_t computeImageDataOffset(const VtfHeaderBuffer& header) {
  const Vtf::Header_70* header70 = header.basicHeader();

  if (header.versionIsAtLeast(7, 3) && header.resourceCount() > 0) {
    uint32_t dataOffset = 0;

    if (!header.findResourceData(Vtf::RESOURCETYPE_IMAGE, dataOffset)) {
      throw AssetException("Could not find VTF resource for high-res image data");
    }

    return dataOffset + Vtf::computeSubImageOffset(*header70, 0, 0, 0);
  } else {
    return Vtf::computeHighResImageDataOffsetSimple(*header70) +
           Vtf::computeSubImageOffset(*header70, 0, 0, 0);
  }
}

VmtTextureReader::VmtTextureReader(
  const NameStrategy& nameStrategy, const FileSystem& fs, Logger& logger)
  : TextureReader(nameStrategy, fs, logger) {}

Assets::Texture VmtTextureReader::doReadTexture(std::shared_ptr<File> file) const {
  auto reader = file->reader();
  const std::string baseTexture = getBaseTextureFromKeyValues(reader);

  if (baseTexture.empty()) {
    throw AssetException("Could not identify base texture for '" + file->path().asString() + "'");
  }

  const Path baseTexturePath = Path("materials/" + baseTexture + ".vtf");

  if (!m_fs.fileExists(baseTexturePath)) {
    throw AssetException(baseTexturePath.asString() + "' does not exist.");
  }

  std::shared_ptr<File> vtfFile;

  try {
    vtfFile = m_fs.openFile(baseTexturePath);
  } catch (const FileSystemException& ex) {
    throw AssetException(std::string(ex.what()));
  }

  return readTextureFromVtf(vtfFile);
}

Assets::Texture VmtTextureReader::readTextureFromVtf(std::shared_ptr<File> file) const {
  auto reader = file->reader();
  VtfHeaderBuffer headerBuffer(reader.subReaderFromBegin(0));

  const Vtf::Header_70* header70 = headerBuffer.basicHeader();

  if (std::memcmp(header70->typeString, Vtf::FILE_SIGNATURE, sizeof(header70->typeString)) != 0) {
    throw AssetException("VTF signature for '" + file->path().asString() + "' was incorrect");
  }

  switch (header70->imageFormat) {
    case Vtf::IMAGE_FORMAT_RGBA8888: {
      return readTexture_RegularUncompressed(headerBuffer, file, GL_RGBA);
    }
    case Vtf::IMAGE_FORMAT_RGB888:
    case Vtf::IMAGE_FORMAT_RGB888_BLUESCREEN: {
      return readTexture_RegularUncompressed(headerBuffer, file, GL_RGB);
    }
    case Vtf::IMAGE_FORMAT_BGRA8888: {
      return readTexture_RegularUncompressed(headerBuffer, file, GL_BGRA);
    }
    case Vtf::IMAGE_FORMAT_BGR888:
    case Vtf::IMAGE_FORMAT_BGR888_BLUESCREEN: {
      return readTexture_RegularUncompressed(headerBuffer, file, GL_BGR);
    }
    case Vtf::IMAGE_FORMAT_DXT1: {
      return readTexture_DXT(headerBuffer, file, &Vtf::decompressDXT1);
    }
    case Vtf::IMAGE_FORMAT_DXT3: {
      return readTexture_DXT(headerBuffer, file, &Vtf::decompressDXT3);
    }
    case Vtf::IMAGE_FORMAT_DXT5: {
      return readTexture_DXT(headerBuffer, file, &Vtf::decompressDXT5);
    }
    default: {
      const Vtf::ImageFormatInfo* info =
        Vtf::getImageFormatInfo(static_cast<Vtf::ImageFormat>(header70->imageFormat));
      const std::string formatName(info ? info->name : "UNKNOWN");
      throw AssetException(
        "VTF image format '" + formatName + "' for '" + file->path().asString() +
        "' was unknown or unsupported");
    }
  }
}

Assets::Texture VmtTextureReader::readTexture_RegularUncompressed(
  const VtfHeaderBuffer& header, std::shared_ptr<File> file, unsigned int format) const {
  const Vtf::Header_70* header70 = header.basicHeader();

  size_t offset = computeImageDataOffset(header);
  auto reader = file->reader();
  reader.seekFromBegin(offset);

  const size_t mipSize = Vtf::computeMipmapSize(
    header70->width, header70->height, 1, 0, static_cast<Vtf::ImageFormat>(header70->imageFormat));

  Assets::TextureBuffer texBuffer(mipSize);
  reader.read(reinterpret_cast<char*>(texBuffer.data()), texBuffer.size());

  // TODO: Compute average colour?
  // TODO: Determine if the texture should be opaque or not?
  return Assets::Texture(
    textureName(file->path()), header70->width, header70->height, Color(0.0f, 0.0f, 0.0f, 1.0f),
    std::move(texBuffer), format, Assets::TextureType::Opaque);
}

Assets::Texture VmtTextureReader::readTexture_DXT(
  const VtfHeaderBuffer& header, std::shared_ptr<File> file, DXTDecompressFunc decompFunc) const {
  ensure(decompFunc, "No decompression function provided");

  const Vtf::Header_70* header70 = header.basicHeader();

  size_t offset = computeImageDataOffset(header);
  auto reader = file->reader();
  reader.seekFromBegin(offset);

  const size_t mipSize = Vtf::computeMipmapSize(
    header70->width, header70->height, 1, 0, static_cast<Vtf::ImageFormat>(header70->imageFormat));

  std::vector<uint8_t> mipData(mipSize);
  reader.read(mipData.data(), mipData.size());

  // TODO: Compute average colour?
  // TODO: Determine if the texture should be opaque or not?
  return Assets::Texture(
    textureName(file->path()), header70->width, header70->height, Color(0.0f, 0.0f, 0.0f, 1.0f),
    decompFunc(mipData.data(), mipData.size(), header70->width, header70->height), GL_RGBA,
    Assets::TextureType::Opaque);
}
} // namespace IO
} // namespace TrenchBroom
