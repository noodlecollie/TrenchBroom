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

#include "IO/VtfDefs.h"

namespace TrenchBroom {
namespace IO {
namespace Vtf {
const ImageFormatInfo* getImageFormatInfo(ImageFormat imageFormat) {
  static const ImageFormatInfo INFO_LIST[] = {
    {IMAGE_FORMAT_RGBA8888, "RGBA8888", 32, 4, 8, 8, 8, 8, false},
    {IMAGE_FORMAT_ABGR8888, "ABGR8888", 32, 4, 8, 8, 8, 8, false},
    {IMAGE_FORMAT_RGB888, "RGB888", 24, 3, 8, 8, 8, 0, false},
    {IMAGE_FORMAT_BGR888, "BGR888", 24, 3, 8, 8, 8, 0, false},
    {IMAGE_FORMAT_RGB565, "RGB565", 16, 2, 5, 6, 5, 0, false},
    {IMAGE_FORMAT_I8, "I8", 8, 1, 0, 0, 0, 0, false},
    {IMAGE_FORMAT_IA88, "IA88", 16, 2, 0, 0, 0, 8, false},
    {IMAGE_FORMAT_P8, "P8", 8, 1, 0, 0, 0, 0, false},
    {IMAGE_FORMAT_A8, "A8", 8, 1, 0, 0, 0, 8, false},
    {IMAGE_FORMAT_RGB888_BLUESCREEN, "RGB888 Bluescreen", 24, 3, 8, 8, 8, 0, false},
    {IMAGE_FORMAT_BGR888_BLUESCREEN, "BGR888 Bluescreen", 24, 3, 8, 8, 8, 0, false},
    {IMAGE_FORMAT_ARGB8888, "ARGB8888", 32, 4, 8, 8, 8, 8, false},
    {IMAGE_FORMAT_BGRA8888, "BGRA8888", 32, 4, 8, 8, 8, 8, false},
    {IMAGE_FORMAT_DXT1, "DXT1", 4, 0, 0, 0, 0, 0, true},
    {IMAGE_FORMAT_DXT3, "DXT3", 8, 0, 0, 0, 0, 8, true},
    {IMAGE_FORMAT_DXT5, "DXT5", 8, 0, 0, 0, 0, 8, true},
    {IMAGE_FORMAT_BGRX8888, "BGRX8888", 32, 4, 8, 8, 8, 0, false},
    {IMAGE_FORMAT_BGR565, "BGR565", 16, 2, 5, 6, 5, 0, false},
    {IMAGE_FORMAT_BGRX5551, "BGRX5551", 16, 2, 5, 5, 5, 0, false},
    {IMAGE_FORMAT_BGRA4444, "BGRA4444", 16, 2, 4, 4, 4, 4, false},
    {IMAGE_FORMAT_DXT1_ONEBITALPHA, "DXT1 One Bit Alpha", 4, 0, 0, 0, 0, 1, true},
    {IMAGE_FORMAT_BGRA5551, "BGRA5551", 16, 2, 5, 5, 5, 1, false},
    {IMAGE_FORMAT_UV88, "UV88", 16, 2, 8, 8, 0, 0, false},
    {IMAGE_FORMAT_UVWQ8888, "UVWQ8888", 32, 4, 8, 8, 8, 8, false},
    {IMAGE_FORMAT_RGBA16161616F, "RGBA16161616F", 64, 8, 16, 16, 16, 16, false},
    {IMAGE_FORMAT_RGBA16161616, "RGBA16161616", 64, 8, 16, 16, 16, 16, false},
    {IMAGE_FORMAT_UVLX8888, "UVLX8888", 32, 4, 8, 8, 8, 8, false},
    {IMAGE_FORMAT_R32F, "R32F", 32, 4, 32, 0, 0, 0, false},
    {IMAGE_FORMAT_RGB323232F, "RGB323232F", 96, 12, 32, 32, 32, 0, false},
    {IMAGE_FORMAT_RGBA32323232F, "RGBA32323232F", 128, 16, 32, 32, 32, 32, false},
    {IMAGE_FORMAT_NV_DST16, "nVidia DST16", 16, 2, 0, 0, 0, 0, false},
    {IMAGE_FORMAT_NV_DST24, "nVidia DST24", 24, 3, 0, 0, 0, 0, false},
    {IMAGE_FORMAT_NV_INTZ, "nVidia INTZ", 32, 4, 0, 0, 0, 0, false},
    {IMAGE_FORMAT_NV_RAWZ, "nVidia RAWZ", 32, 4, 0, 0, 0, 0, false},
    {IMAGE_FORMAT_ATI_DST16, "ATI DST16", 16, 2, 0, 0, 0, 0, false},
    {IMAGE_FORMAT_ATI_DST24, "ATI DST24", 24, 3, 0, 0, 0, 0, false},
    {IMAGE_FORMAT_NV_NULL, "nVidia NULL", 32, 4, 0, 0, 0, 0, false},
    {IMAGE_FORMAT_ATI2N, "ATI1N", 4, 0, 0, 0, 0, 0, true},
    {IMAGE_FORMAT_ATI1N, "ATI2N", 8, 0, 0, 0, 0, 0, true},
    {IMAGE_FORMAT_NONE, "NONE", 0, 0, 0, 0, 0, 0, false}, // Must be at the end
  };

  for (size_t index = 0; INFO_LIST[index].formatValue != IMAGE_FORMAT_NONE; ++index) {
    if (INFO_LIST[index].formatValue == imageFormat) {
      return &INFO_LIST[index];
    }
  }

  return nullptr;
}
} // namespace Vtf
} // namespace IO
} // namespace TrenchBroom
