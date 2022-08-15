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

#include "Assets/TextureBuffer.h"
#include "IO/VtfDefs.h"
#include "vecmath/forward.h"
#include "vecmath/vec.h"
#include <cstdint>
#include <limits>

namespace TrenchBroom {
namespace IO {
namespace Vtf {
// Much of this is borrowed from https://github.com/panzi/VTFLib

uint32_t computeFaceCount(const Header_70& header);

// Computes where in the file the main VTF image data begins.
// Only valid for versions up to 7.2 - a FileFormatException
// will be thrown otherwise.
size_t computeHighResImageDataOffsetSimple(const Header_70& header);

// Calculates where in the main VTF image data the specific sub-image begins.
size_t computeSubImageOffset(
  const Header_72& header, uint32_t frameIndex, uint32_t faceIndex, uint32_t depthIndex,
  uint32_t mipmapLevel);

// Same as above but assumes that depth is 1.
size_t computeSubImageOffset(
  const Header_70& header, uint32_t frameIndex, uint32_t faceIndex, uint32_t mipmapLevel);

// Computes the number of bytes required for a mipmap with the given parameters.
// Width and height are the width and height of the image; depth is the depth (ie. the number
// of "slices" of (width * height) pixels); mipmap level is how many reductions have been performed
// to the mipmap (0 = the original image); image format is the format of the pixels in the image.
size_t computeMipmapSize(
  size_t width, size_t height, size_t depth, uint32_t mipmapLevel, ImageFormat imageFormat);

// Computes the dimensions for a mipmap power of an image with a specific width, height, and depth.
vm::vec3s computeMipmapDimensions(size_t width, size_t height, size_t depth, uint32_t mipmapLevel);

// Computes the dimensions for a mipmap power of an image with a specific width and height;
vm::vec2s computeMipmapDimensions(size_t width, size_t height, uint32_t mipmapLevel);

// Computes the number of bytes required for a width * height * depth image of the given format.
size_t computeImageSize(size_t width, size_t height, size_t depth, ImageFormat imageFormat);

Assets::TextureBuffer decompressDXT1(const void* in, size_t inLength, size_t width, size_t height);
Assets::TextureBuffer decompressDXT3(const void* in, size_t inLength, size_t width, size_t height);
Assets::TextureBuffer decompressDXT5(const void* in, size_t inLength, size_t width, size_t height);
} // namespace Vtf
} // namespace IO
} // namespace TrenchBroom
