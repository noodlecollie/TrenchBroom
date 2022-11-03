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
#include "Assets/TextureBuffer.h"
#include "Ensure.h"
#include "Exceptions.h"
#include "IO/File.h"
#include "IO/FileSystem.h"
#include "IO/ValveKeyValuesParser.h"
#include "IO/ValveKeyValuesTree.h"
#include "IO/VtfDefs.h"
#include "IO/VtfHeaderBuffer.h"
#include "IO/VtfUtils.h"
#include "Logger.h"
#include "kdl/string_compare.h"
#include <cstdint>
#include <cstring>

namespace TrenchBroom {
namespace IO {
static bool isSupportedMaterialShader(const std::string& shader) {
  return kdl::ci::str_compare(shader, "LightmappedGeneric") ||
         kdl::ci::str_compare(shader, "LightmappedReflective") ||
         kdl::ci::str_compare(shader, "WorldTwoTextureBlend") ||
         kdl::ci::str_compare(shader, "WorldVertexTransition") ||
         kdl::ci::str_compare(shader, "VertexLitGeneric") ||
         kdl::ci::str_compare(shader, "Water") || kdl::ci::str_compare(shader, "UnlitGeneric");
}

static ValveKeyValuesTree readKVFile(Logger& logger, const std::shared_ptr<File>& file) {
  if (!file) {
    throw AssetException("VMT file was not valid");
  }

  BufferedReader reader = file->reader().buffer();
  ValveKeyValuesTree tree;

  try {
    ValveKeyValuesParser kvParser(reader.stringView());
    kvParser.parse(logger, tree);
  } catch (const ParserException& ex) {
    throw AssetException(ex.what());
  }

  return tree;
}

static std::string getBaseTexture(ValveKeyValuesNode* material) {
  ensure(material, "Material node was null");

  if (!isSupportedMaterialShader(material->getKey())) {
    throw AssetException(
      "Could not obtain base texture: material type '" + material->getKey() +
      "' is not supported.");
  }

  ValveKeyValuesNode* baseTexture = material->findChildByKey("$basetexture");

  if (!baseTexture) {
    // Check to see if this is an eye.
    baseTexture = material->findChildByKey("$iris");

    if (!baseTexture) {
      throw AssetException("Could not obtain base texture: no entry was found in material.");
    }
  }

  const std::string baseTextureStr = baseTexture->getValueString();

  if (baseTextureStr.empty()) {
    throw AssetException("Could not obtain base texture: entry was empty.");
  }

  return baseTextureStr;
}

static bool shouldIgnoreAlphaChannel(ValveKeyValuesNode* material) {
  ensure(material, "Material node was null");

  // Is the alpha channel being used as a mask of some kind?
  if (
    material->hasChildWithBooleanValue("$basealphaenvmapmask") ||
    material->hasChildWithBooleanValue("$selfillum") ||
    material->hasChildWithBooleanValue("$basemapalphaphongmask")) {
    return true;
  }

  // Is the material used for a character eye?
  if (material->findChildByKey("$iris")) {
    return true;
  }

  return false;
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

static Color getAverageColour4Channel(const Assets::TextureBuffer& buffer, const GLenum format) {
  ensure(format == GL_RGBA || format == GL_BGRA, "expected RGBA or BGRA");

  const unsigned char* const data = buffer.data();
  const std::size_t bufferSize = buffer.size();

  Color average;
  for (std::size_t i = 0; i < bufferSize; i += 4) {
    average = average + Color(data[i], data[i + 1], data[i + 2], data[i + 3]);
  }
  const std::size_t numPixels = bufferSize / 4;
  average = average / static_cast<float>(numPixels);

  return average;
}

static Color getAverageColour3Channel(const Assets::TextureBuffer& buffer, const GLenum format) {
  ensure(format == GL_RGB || format == GL_BGR, "expected RGB or BGR");

  const unsigned char* const data = buffer.data();
  const std::size_t bufferSize = buffer.size();

  Color average;
  for (std::size_t i = 0; i < bufferSize; i += 3) {
    average = average + Color(data[i], data[i + 1], data[i + 2]);
  }
  const std::size_t numPixels = bufferSize / 3;
  average = average / static_cast<float>(numPixels);

  return average;
}

VmtTextureReader::VmtTextureReader(
  const NameStrategy& nameStrategy, const FileSystem& fs, Logger& logger)
  : TextureReader(nameStrategy, fs, logger) {}

Assets::Texture VmtTextureReader::doReadTexture(std::shared_ptr<File> file) const {
  ValveKeyValuesTree tree = readKVFile(m_logger, file);
  ValveKeyValuesNode* material = tree.getRoot()->getChild(0);

  if (!material) {
    throw AssetException("Could not obtain base texture: file was empty");
  }

  const std::string baseTexture = getBaseTexture(material);
  const bool ignoreAlpha = shouldIgnoreAlphaChannel(material);

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

  return readTextureFromVtf(vtfFile, ignoreAlpha);
}

Assets::Texture VmtTextureReader::readTextureFromVtf(
  std::shared_ptr<File> file, bool wipeAlphaChannel) const {
  auto reader = file->reader();
  VtfHeaderBuffer headerBuffer(reader.subReaderFromBegin(0));

  const Vtf::Header_70* header70 = headerBuffer.basicHeader();

  if (std::memcmp(header70->typeString, Vtf::FILE_SIGNATURE, sizeof(header70->typeString)) != 0) {
    throw AssetException("VTF signature for '" + file->path().asString() + "' was incorrect");
  }

  Assets::TextureBuffer textureBuffer;
  unsigned int format = GL_RGBA;

  switch (header70->imageFormat) {
    case Vtf::IMAGE_FORMAT_RGBA8888: {
      format = GL_RGBA;
      textureBuffer = readTexture_RegularUncompressed(headerBuffer, file);
      break;
    }
    case Vtf::IMAGE_FORMAT_RGB888:
    case Vtf::IMAGE_FORMAT_RGB888_BLUESCREEN: {
      format = GL_RGB;
      textureBuffer = readTexture_RegularUncompressed(headerBuffer, file);
      break;
    }
    case Vtf::IMAGE_FORMAT_BGRA8888: {
      format = GL_BGRA;
      textureBuffer = readTexture_RegularUncompressed(headerBuffer, file);
      break;
    }
    case Vtf::IMAGE_FORMAT_BGR888:
    case Vtf::IMAGE_FORMAT_BGR888_BLUESCREEN: {
      format = GL_BGR;
      textureBuffer = readTexture_RegularUncompressed(headerBuffer, file);
      break;
    }
    case Vtf::IMAGE_FORMAT_DXT1: {
      format = GL_RGBA;
      textureBuffer = readTexture_DXT(headerBuffer, file, &Vtf::decompressDXT1);
      break;
    }
    case Vtf::IMAGE_FORMAT_DXT3: {
      format = GL_RGBA;
      textureBuffer = readTexture_DXT(headerBuffer, file, &Vtf::decompressDXT3);
      break;
    }
    case Vtf::IMAGE_FORMAT_DXT5: {
      format = GL_RGBA;
      textureBuffer = readTexture_DXT(headerBuffer, file, &Vtf::decompressDXT5);
      break;
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

  const Color avgColour =
    postProcessTextureAndComputeAvgColour(textureBuffer, format, wipeAlphaChannel);

  return Assets::Texture(
    textureName(file->path()), header70->width, header70->height, avgColour,
    std::move(textureBuffer), format,
    wipeAlphaChannel ? Assets::TextureType::Opaque : Assets::TextureType::Masked);
}

Assets::TextureBuffer VmtTextureReader::readTexture_RegularUncompressed(
  const VtfHeaderBuffer& header, std::shared_ptr<File> file) const {
  const Vtf::Header_70* header70 = header.basicHeader();

  size_t offset = computeImageDataOffset(header);
  auto reader = file->reader();
  reader.seekFromBegin(offset);

  const size_t mipSize = Vtf::computeMipmapSize(
    header70->width, header70->height, 1, 0, static_cast<Vtf::ImageFormat>(header70->imageFormat));

  Assets::TextureBuffer texBuffer(mipSize);
  reader.read(reinterpret_cast<char*>(texBuffer.data()), texBuffer.size());

  return texBuffer;
}

Assets::TextureBuffer VmtTextureReader::readTexture_DXT(
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

  return decompFunc(mipData.data(), mipData.size(), header70->width, header70->height);
}

Color VmtTextureReader::postProcessTextureAndComputeAvgColour(
  Assets::TextureBuffer& buffer, unsigned int format, bool wipeAlphaChannel) const {

  if (format == GL_RGBA || format == GL_BGRA) {
    if (wipeAlphaChannel) {
      const size_t bufferSize = buffer.size();
      unsigned char* bufferData = buffer.data();

      for (size_t index = 3; index < bufferSize; index += 4) {
        bufferData[index] = 0xFF;
      }
    }

    return getAverageColour4Channel(buffer, format);
  } else if (format == GL_RGB || format == GL_BGR) {
    // No alpha to manipulate
    return getAverageColour3Channel(buffer, format);
  } else {
    throw AssetException("Unsupported GL texture format encountered");
  }
}
} // namespace IO
} // namespace TrenchBroom
