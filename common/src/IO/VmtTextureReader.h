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

#include "Assets/Texture.h"
#include "Color.h"
#include "IO/TextureReader.h"

namespace TrenchBroom {
class Logger;

namespace Assets {
class TextureBuffer;
}

namespace IO {
class VtfHeaderBuffer;

namespace Vtf {
struct Header_70;
}

class VmtTextureReader : public TextureReader {
public:
  VmtTextureReader(const NameStrategy& nameStrategy, const FileSystem& fs, Logger& logger);

private:
  using DXTDecompressFunc =
    Assets::TextureBuffer (*)(const void* in, size_t inLength, size_t width, size_t height);

  Assets::Texture doReadTexture(std::shared_ptr<File> file) const override;
  Assets::Texture readTextureFromVtf(std::shared_ptr<File> file, bool wipeAlphaChannel) const;

  Assets::TextureBuffer readTexture_RegularUncompressed(
    const VtfHeaderBuffer& header, std::shared_ptr<File> file) const;
  Assets::TextureBuffer readTexture_DXT(
    const VtfHeaderBuffer& header, std::shared_ptr<File> file, DXTDecompressFunc decompFunc) const;

  Color postProcessTextureAndComputeAvgColour(
    Assets::TextureBuffer& buffer, unsigned int format, bool wipeAlphaChannel) const;
};
} // namespace IO
} // namespace TrenchBroom
