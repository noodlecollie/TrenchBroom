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

#include "IO/VtfUtils.h"
#include "Ensure.h"
#include "Exceptions.h"

namespace TrenchBroom {
namespace IO {
namespace Vtf {
static constexpr uint32_t VTF_MINOR_VERSION_MIN_NO_SPHERE_MAP = 5;

// RGBA Pixel type
struct Colour8888 {
  uint8_t r; // change the order of names to change the
  uint8_t g; // order of the output ARGB or BGRA, etc...
  uint8_t b; // Last one is MSB, 1st is LSB.
  uint8_t a;
};

// RGB Pixel type
struct Colour888 {
  uint8_t r; // change the order of names to change the
  uint8_t g; // order of the output ARGB or BGRA, etc...
  uint8_t b; // Last one is MSB, 1st is LSB.
};

// BGR565 Pixel type
struct Colour565 {
  uint32_t nBlue : 5;  // order of names changes
  uint32_t nGreen : 6; // byte order of output to 32 bit
  uint32_t nRed : 5;
};

// DXTn Alpha block types
struct DXTAlphaBlockExplicit {
  int16_t row[4];
};

// DXT* textures must have width and height as multiples of 4.
static constexpr vm::vec2s dxtDimensionsForImage(size_t width, size_t height) {
  return vm::vec2s{((width + 3) / 4) * 4, ((height + 3) / 4) * 4};
}

static void validateInputsForDXTDecompression(
  const std::string& desc, const void* in, size_t inLength, size_t width, size_t height,
  size_t bytesPerBlock) {
  if (!in) {
    throw AssetException("Could not decompress " + desc + " texture: input data was null");
  }

  if (width < 1) {
    throw AssetException("Could not decompress " + desc + " texture: input image width was zero");
  }

  if (height < 1) {
    throw AssetException("Could not decompress " + desc + " texture: input image height was zero");
  }

  const vm::vec2s expectedInputDims = dxtDimensionsForImage(width, height);
  const size_t expectedInputBlocks = (expectedInputDims[0] / 4) * (expectedInputDims[1] / 4);
  const size_t expectedInputBytes = expectedInputBlocks * bytesPerBlock;

  if (inLength < expectedInputBytes) {
    throw AssetException(
      "Could not decompress " + desc + " texture: input data length of " +
      std::to_string(inLength) + " bytes did not meet minimum length of " +
      std::to_string(expectedInputBytes) + " bytes");
  }
}

static size_t computeSubImageOffsetInternal(
  const Header_70& header, uint32_t frameIndex, uint32_t faceIndex, uint32_t depthIndex,
  uint32_t mipmapLevel, uint32_t maxDepth) {
  size_t offset = 0;

  const uint32_t frameCount = header.frames;
  const uint32_t faceCount = computeFaceCount(header);
  const uint32_t mipCount = header.mipCount;

  if (frameCount < 1) {
    throw FileFormatException("Invalid VTF file with 0 frames");
  }

  if (faceCount < 1) {
    throw FileFormatException("Invalid VTF file with 0 faces");
  }

  if (mipCount < 1) {
    throw FileFormatException("Invalid VTF file with 0 mipmaps");
  }

  if (maxDepth < 1) {
    throw FileFormatException("Invalid VTF file with 0 depth slices");
  }

  // Clamp for sanity:
  frameIndex = std::min<uint32_t>(frameCount - 1, frameIndex);
  faceIndex = std::min<uint32_t>(faceCount - 1, faceIndex);
  depthIndex = std::min<uint32_t>(maxDepth - 1, depthIndex);
  mipmapLevel = std::min<uint32_t>(mipCount - 1, mipmapLevel);

  // Transverse past all frames and faces of each mipmap (up to the requested one).
  // This means that for every mipmap before the one we want, we skip (mipmapSize * totalFrames *
  // totalFaces) bytes.
  for (uint32_t mipIndex = mipCount - 1; mipIndex > mipmapLevel; --mipIndex) {
    offset += computeMipmapSize(
                header.width, header.height, maxDepth, mipIndex,
                static_cast<ImageFormat>(header.imageFormat)) *
              frameCount * faceCount;
  }

  // Compute the number of bytes required for the requested mipmap, including all depth slices.
  const size_t mipmapSizeForAllDepthSlices = computeMipmapSize(
    header.width, header.height, maxDepth, mipmapLevel,
    static_cast<ImageFormat>(header.imageFormat));

  // Compute the number of bytes for the requested mipmap again, but this time only for a single
  // slice.
  const size_t mipmapSizeForSingleDepthSlice = computeMipmapSize(
    header.width, header.height, 1, mipmapLevel, static_cast<ImageFormat>(header.imageFormat));

  // Transverse past requested frames and faces of requested mipmap:
  // 1. Skip all frames before the frame that we want. This skips all faces and all depth slices of
  // these unwanted frames.
  offset += mipmapSizeForAllDepthSlices * frameIndex * faceCount * maxDepth;

  // 2. Skip all faces before the face that we want. This skips all depth slices of these unwanted
  // faces.
  offset += mipmapSizeForAllDepthSlices * faceIndex * maxDepth;

  // 3. Skip all depth slices before the one that we want.
  offset += mipmapSizeForSingleDepthSlice * depthIndex;

  // Return the final offset.
  return offset;
}

uint32_t computeFaceCount(const Header_70& header) {
  // VTFLib's not-very-readable expression:
  //   return this->Header->Flags & TEXTUREFLAGS_ENVMAP
  //            ? (this->Header->StartFrame != 0xffff &&
  //                   this->Header->Version[1] < VTF_MINOR_VERSION_MIN_NO_SPHERE_MAP
  //                 ? CUBEMAP_FACE_COUNT
  //                 : CUBEMAP_FACE_COUNT - 1)
  //            : 1;

  if (!(header.flags & TEXTUREFLAGS_ENVMAP)) {
    // Not a cubemap, so only ever a single face
    return 1;
  }

  if (header.startFrame != 0xFFFF && header.version[1] < VTF_MINOR_VERSION_MIN_NO_SPHERE_MAP) {
    // 7-faced cubemap
    return 7;
  }

  // 6-faced cubemap
  return 6;
}

size_t computeHighResImageDataOffsetSimple(const Header_70& header) {
  if (header.version[0] != 7 || header.version[1] >= 3) {
    throw FileFormatException(
      "Header version out of range for computeHighResImageDataOffsetSimple");
  }

  size_t thumbnailBufferSize = 0;

  if (header.lowResImageFormat != IMAGE_FORMAT_NONE) {
    thumbnailBufferSize = computeImageSize(
      header.lowResImageWidth, header.lowResImageHeight, 1,
      static_cast<ImageFormat>(header.lowResImageFormat));
  }

  return header.headerSize + thumbnailBufferSize;
}

size_t computeSubImageOffset(
  const Header_72& header, uint32_t frameIndex, uint32_t faceIndex, uint32_t depthIndex,
  uint32_t mipmapLevel) {
  return computeSubImageOffsetInternal(
    header, frameIndex, faceIndex, depthIndex, mipmapLevel, header.depth);
}

size_t computeSubImageOffset(
  const Header_70& header, uint32_t frameIndex, uint32_t faceIndex, uint32_t mipmapLevel) {
  return computeSubImageOffsetInternal(header, frameIndex, faceIndex, 0, mipmapLevel, 1);
}

size_t computeMipmapSize(
  size_t width, size_t height, size_t depth, uint32_t mipmapLevel, ImageFormat imageFormat) {
  // Figure out the width/height of this MIP level
  const auto dims = computeMipmapDimensions(width, height, depth, mipmapLevel);

  // Return the memory requirements
  return computeImageSize(dims[0], dims[1], dims[2], imageFormat);
}

vm::vec3s computeMipmapDimensions(size_t width, size_t height, size_t depth, uint32_t mipmapLevel) {
  // Work out the width/height by taking the orignal dimension
  // and bit shifting them down mipmapLevel times
  const size_t outWidth = std::max<size_t>(width >> mipmapLevel, 1);
  const size_t outHeight = std::max<size_t>(height >> mipmapLevel, 1);
  const size_t outDepth = std::max<size_t>(depth >> mipmapLevel, 1);

  return vm::vec3s{outWidth, outHeight, outDepth};
}

vm::vec2s computeMipmapDimensions(size_t width, size_t height, uint32_t mipmapLevel) {
  // const auto out = ComputeMipmapDimensions(width, height, 1, mipmapLevel);
  // return vm::vec2s{out[0], out[1]};
  return vm::slice<2>(computeMipmapDimensions(width, height, 1, mipmapLevel), 0);
}

size_t computeImageSize(size_t width, size_t height, size_t depth, ImageFormat imageFormat) {
  switch (imageFormat) {
    case IMAGE_FORMAT_DXT1:
    case IMAGE_FORMAT_DXT1_ONEBITALPHA: {
      if (width < 4 && width > 0) {
        width = 4;
      }

      if (height < 4 && height > 0) {
        height = 4;
      }

      return ((width + 3) / 4) * ((height + 3) / 4) * 8 * depth;
    }
    case IMAGE_FORMAT_DXT3:
    case IMAGE_FORMAT_DXT5: {
      if (width < 4 && width > 0) {
        width = 4;
      }

      if (height < 4 && height > 0) {
        height = 4;
      }

      return ((width + 3) / 4) * ((height + 3) / 4) * 16 * depth;
    }
    default: {
      const ImageFormatInfo* info = getImageFormatInfo(imageFormat);

      if (!info) {
        throw FileFormatException(
          "Unsupported or unrecognised image format with ID " + std::to_string(imageFormat));
      }

      if (info->bytesPerPixel < 1) {
        throw FileFormatException("Unsupported image format '" + std::string(info->name) + "'");
      }

      return width * height * depth * info->bytesPerPixel;
    }
  }
}

// VTFLib notes:
// DXTn decompression code is based on examples on Microsofts website and from the
// Developers Image Library (http://www.imagelib.org) (c) Denton Woods.
Assets::TextureBuffer decompressDXT1(const void* in, size_t inLength, size_t width, size_t height) {
  static constexpr size_t BYTES_PER_PIXEL = 4;
  static constexpr size_t BYTES_PER_CHANNEL = 1;
  static constexpr size_t BYTES_PER_BLOCK = 8;

  validateInputsForDXTDecompression("DXT1", in, inLength, width, height, BYTES_PER_BLOCK);

  const size_t bytesPerScanLine = BYTES_PER_PIXEL * BYTES_PER_CHANNEL * width;

  Assets::TextureBuffer textureBuffer(width * height * 4);
  unsigned char* dst = textureBuffer.data();

  const uint8_t* inCursor = static_cast<const uint8_t*>(in);

  for (size_t y = 0; y < height; y += 4) {
    for (size_t x = 0; x < width; x += 4) {
      const Colour565* color_0 = reinterpret_cast<const Colour565*>(inCursor);
      const Colour565* color_1 = reinterpret_cast<const Colour565*>(inCursor + 2);
      const uint32_t bitmask = reinterpret_cast<const uint32_t*>(inCursor)[1];

      inCursor += BYTES_PER_BLOCK;

      Colour8888 colours[4];

      colours[0].r = color_0->nRed << 3;
      colours[0].g = color_0->nGreen << 2;
      colours[0].b = color_0->nBlue << 3;
      colours[0].a = 0xFF;

      colours[1].r = color_1->nRed << 3;
      colours[1].g = color_1->nGreen << 2;
      colours[1].b = color_1->nBlue << 3;
      colours[1].a = 0xFF;

      if (
        *(reinterpret_cast<const uint16_t*>(color_0)) >
        *(reinterpret_cast<const uint16_t*>(color_1))) {
        // Four-color block: derive the other two colors.
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
        // These 2-bit codes correspond to the 2-bit fields
        // stored in the 64-bit block.
        colours[2].b = static_cast<uint8_t>((2 * colours[0].b + colours[1].b + 1) / 3);
        colours[2].g = static_cast<uint8_t>((2 * colours[0].g + colours[1].g + 1) / 3);
        colours[2].r = static_cast<uint8_t>((2 * colours[0].r + colours[1].r + 1) / 3);
        colours[2].a = 0xFF;

        colours[3].b = static_cast<uint8_t>((colours[0].b + 2 * colours[1].b + 1) / 3);
        colours[3].g = static_cast<uint8_t>((colours[0].g + 2 * colours[1].g + 1) / 3);
        colours[3].r = static_cast<uint8_t>((colours[0].r + 2 * colours[1].r + 1) / 3);
        colours[3].a = 0xFF;
      } else {
        // Three-color block: derive the other color.
        // 00 = color_0,  01 = color_1,  10 = color_2,
        // 11 = transparent.
        // These 2-bit codes correspond to the 2-bit fields
        // stored in the 64-bit block.
        colours[2].b = static_cast<uint8_t>((colours[0].b + colours[1].b) / 2);
        colours[2].g = static_cast<uint8_t>((colours[0].g + colours[1].g) / 2);
        colours[2].r = static_cast<uint8_t>((colours[0].r + colours[1].r) / 2);
        colours[2].a = 0xFF;

        colours[3].b = static_cast<uint8_t>((colours[0].b + 2 * colours[1].b + 1) / 3);
        colours[3].g = static_cast<uint8_t>((colours[0].g + 2 * colours[1].g + 1) / 3);
        colours[3].r = static_cast<uint8_t>((colours[0].r + 2 * colours[1].r + 1) / 3);
        colours[3].a = 0x00;
      }

      for (size_t j = 0, k = 0; j < 4; j++) {
        for (size_t i = 0; i < 4; i++, k++) {
          const uint32_t select = (bitmask & (0x03 << k * 2)) >> k * 2;
          const Colour8888* col = &colours[select];

          if (((x + i) < width) && ((y + j) < height)) {
            const size_t offset = (y + j) * bytesPerScanLine + (x + i) * BYTES_PER_PIXEL;
            dst[offset + 0] = col->r;
            dst[offset + 1] = col->g;
            dst[offset + 2] = col->b;
            dst[offset + 3] = col->a;
          }
        }
      }
    }
  }

  return textureBuffer;
}

Assets::TextureBuffer decompressDXT3(const void* in, size_t inLength, size_t width, size_t height) {
  static constexpr size_t BYTES_PER_PIXEL = 4;
  static constexpr size_t BYTES_PER_CHANNEL = 1;
  static constexpr size_t COLOUR_BYTES_PER_BLOCK = 8;
  static constexpr size_t ALPHA_BYTES_PER_BLOCK = 8;
  static constexpr size_t BYTES_PER_BLOCK = COLOUR_BYTES_PER_BLOCK + ALPHA_BYTES_PER_BLOCK;

  validateInputsForDXTDecompression("DXT3", in, inLength, width, height, BYTES_PER_BLOCK);

  const size_t bytesPerScanLine = BYTES_PER_PIXEL * BYTES_PER_CHANNEL * width;

  Assets::TextureBuffer textureBuffer(width * height * 4);
  unsigned char* dst = textureBuffer.data();

  const uint8_t* inCursor = static_cast<const uint8_t*>(in);

  for (size_t y = 0; y < height; y += 4) {
    for (size_t x = 0; x < width; x += 4) {
      const DXTAlphaBlockExplicit* alpha = reinterpret_cast<const DXTAlphaBlockExplicit*>(inCursor);

      inCursor += ALPHA_BYTES_PER_BLOCK;

      const Colour565* color_0 = reinterpret_cast<const Colour565*>(inCursor);
      const Colour565* color_1 = reinterpret_cast<const Colour565*>(inCursor + 2);
      const uint32_t bitmask = reinterpret_cast<const uint32_t*>(inCursor)[1];

      inCursor += COLOUR_BYTES_PER_BLOCK;

      Colour8888 colours[4];

      colours[0].r = color_0->nRed << 3;
      colours[0].g = color_0->nGreen << 2;
      colours[0].b = color_0->nBlue << 3;
      colours[0].a = 0xFF;

      colours[1].r = color_1->nRed << 3;
      colours[1].g = color_1->nGreen << 2;
      colours[1].b = color_1->nBlue << 3;
      colours[1].a = 0xFF;

      // Four-color block: derive the other two colors.
      // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
      // These 2-bit codes correspond to the 2-bit fields
      // stored in the 64-bit block.
      colours[2].b = static_cast<uint8_t>((2 * colours[0].b + colours[1].b + 1) / 3);
      colours[2].g = static_cast<uint8_t>((2 * colours[0].g + colours[1].g + 1) / 3);
      colours[2].r = static_cast<uint8_t>((2 * colours[0].r + colours[1].r + 1) / 3);
      colours[2].a = 0xFF;

      colours[3].b = static_cast<uint8_t>((colours[0].b + 2 * colours[1].b + 1) / 3);
      colours[3].g = static_cast<uint8_t>((colours[0].g + 2 * colours[1].g + 1) / 3);
      colours[3].r = static_cast<uint8_t>((colours[0].r + 2 * colours[1].r + 1) / 3);
      colours[3].a = 0xFF;

      for (size_t j = 0, k = 0; j < 4; j++) {
        for (size_t i = 0; i < 4; i++, k++) {
          const size_t select = (bitmask & (0x03 << k * 2)) >> k * 2;
          const Colour8888* col = &colours[select];

          if (((x + i) < width) && ((y + j) < height)) {
            const size_t offset = (y + j) * bytesPerScanLine + (x + i) * BYTES_PER_PIXEL;
            dst[offset + 0] = col->r;
            dst[offset + 1] = col->g;
            dst[offset + 2] = col->b;
          }
        }
      }

      for (size_t j = 0; j < 4; j++) {
        int16_t word = alpha->row[j];

        for (size_t i = 0; i < 4; i++) {
          if (((x + i) < width) && ((y + j) < height)) {
            const size_t offset = (y + j) * bytesPerScanLine + (x + i) * BYTES_PER_PIXEL + 3;
            dst[offset] = static_cast<uint8_t>(word & 0x0F);
            dst[offset] = static_cast<uint8_t>(dst[offset] | (dst[offset] << 4));
          }

          word >>= 4;
        }
      }
    }
  }

  return textureBuffer;
}

Assets::TextureBuffer decompressDXT5(const void* in, size_t inLength, size_t width, size_t height) {
  static constexpr size_t BYTES_PER_PIXEL = 4;
  static constexpr size_t BYTES_PER_CHANNEL = 1;
  static constexpr size_t COLOUR_BYTES_PER_BLOCK = 8;
  static constexpr size_t ALPHA_BYTES_PER_BLOCK = 8;
  static constexpr size_t BYTES_PER_BLOCK = COLOUR_BYTES_PER_BLOCK + ALPHA_BYTES_PER_BLOCK;

  validateInputsForDXTDecompression("DXT5", in, inLength, width, height, BYTES_PER_BLOCK);

  const size_t bytesPerScanLine = BYTES_PER_PIXEL * BYTES_PER_CHANNEL * width;

  Assets::TextureBuffer textureBuffer(width * height * 4);
  unsigned char* dst = textureBuffer.data();

  const uint8_t* inCursor = static_cast<const uint8_t*>(in);

  for (size_t y = 0; y < height; y += 4) {
    for (size_t x = 0; x < width; x += 4) {
      uint8_t alphas[8];

      alphas[0] = inCursor[0];
      alphas[1] = inCursor[1];
      const uint8_t* alphamask = inCursor + 2;

      inCursor += ALPHA_BYTES_PER_BLOCK;

      const Colour565* color_0 = reinterpret_cast<const Colour565*>(inCursor);
      const Colour565* color_1 = reinterpret_cast<const Colour565*>(inCursor + 2);
      const uint32_t bitmask = reinterpret_cast<const uint32_t*>(inCursor)[1];

      inCursor += COLOUR_BYTES_PER_BLOCK;

      Colour8888 colours[4];

      colours[0].r = color_0->nRed << 3;
      colours[0].g = color_0->nGreen << 2;
      colours[0].b = color_0->nBlue << 3;
      colours[0].a = 0xFF;

      colours[1].r = color_1->nRed << 3;
      colours[1].g = color_1->nGreen << 2;
      colours[1].b = color_1->nBlue << 3;
      colours[1].a = 0xFF;

      // Four-color block: derive the other two colors.
      // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
      // These 2-bit codes correspond to the 2-bit fields
      // stored in the 64-bit block.
      colours[2].b = static_cast<uint8_t>((2 * colours[0].b + colours[1].b + 1) / 3);
      colours[2].g = static_cast<uint8_t>((2 * colours[0].g + colours[1].g + 1) / 3);
      colours[2].r = static_cast<uint8_t>((2 * colours[0].r + colours[1].r + 1) / 3);
      colours[2].a = 0xFF;

      colours[3].b = static_cast<uint8_t>((colours[0].b + 2 * colours[1].b + 1) / 3);
      colours[3].g = static_cast<uint8_t>((colours[0].g + 2 * colours[1].g + 1) / 3);
      colours[3].r = static_cast<uint8_t>((colours[0].r + 2 * colours[1].r + 1) / 3);
      colours[3].a = 0xFF;

      for (size_t j = 0, k = 0; j < 4; j++) {
        for (size_t i = 0; i < 4; i++, k++) {
          const size_t select = (bitmask & (0x03 << k * 2)) >> k * 2;
          const Colour8888* col = &colours[select];

          // only put pixels out < width or height
          if (((x + i) < width) && ((y + j) < height)) {
            const size_t offset = (y + j) * bytesPerScanLine + (x + i) * BYTES_PER_PIXEL;
            dst[offset + 0] = col->r;
            dst[offset + 1] = col->g;
            dst[offset + 2] = col->b;
          }
        }
      }

      // 8-alpha or 6-alpha block?
      if (alphas[0] > alphas[1]) {
        // 8-alpha block:  derive the other six alphas.
        // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
        alphas[2] = static_cast<uint8_t>((6 * alphas[0] + 1 * alphas[1] + 3) / 7); // bit code 010
        alphas[3] = static_cast<uint8_t>((5 * alphas[0] + 2 * alphas[1] + 3) / 7); // bit code 011
        alphas[4] = static_cast<uint8_t>((4 * alphas[0] + 3 * alphas[1] + 3) / 7); // bit code 100
        alphas[5] = static_cast<uint8_t>((3 * alphas[0] + 4 * alphas[1] + 3) / 7); // bit code 101
        alphas[6] = static_cast<uint8_t>((2 * alphas[0] + 5 * alphas[1] + 3) / 7); // bit code 110
        alphas[7] = static_cast<uint8_t>((1 * alphas[0] + 6 * alphas[1] + 3) / 7); // bit code 111
      } else {
        // 6-alpha block.
        // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
        alphas[2] = static_cast<uint8_t>((4 * alphas[0] + 1 * alphas[1] + 2) / 5); // Bit code 010
        alphas[3] = static_cast<uint8_t>((3 * alphas[0] + 2 * alphas[1] + 2) / 5); // Bit code 011
        alphas[4] = static_cast<uint8_t>((2 * alphas[0] + 3 * alphas[1] + 2) / 5); // Bit code 100
        alphas[5] = static_cast<uint8_t>((1 * alphas[0] + 4 * alphas[1] + 2) / 5); // Bit code 101
        alphas[6] = 0x00;                                                          // Bit code 110
        alphas[7] = 0xFF;                                                          // Bit code 111
      }

      // Note: Have to separate the next two loops,
      // it operates on a 6-byte system.

      // First three bytes
      int bits = *reinterpret_cast<const int*>(alphamask);

      for (size_t j = 0; j < 2; j++) {
        for (size_t i = 0; i < 4; i++) {
          // only put pixels out < width or height
          if (((x + i) < width) && ((y + j) < height)) {
            const size_t offset = (y + j) * bytesPerScanLine + (x + i) * BYTES_PER_PIXEL + 3;
            dst[offset] = alphas[bits & 0x07];
          }
          bits >>= 3;
        }
      }

      // Last three bytes
      bits = *reinterpret_cast<const int*>(&alphamask[3]);

      for (size_t j = 2; j < 4; j++) {
        for (size_t i = 0; i < 4; i++) {
          // only put pixels out < width or height
          if (((x + i) < width) && ((y + j) < height)) {
            const size_t offset = (y + j) * bytesPerScanLine + (x + i) * BYTES_PER_PIXEL + 3;
            dst[offset] = alphas[bits & 0x07];
          }
          bits >>= 3;
        }
      }
    }
  }

  return textureBuffer;
}
} // namespace Vtf
} // namespace IO
} // namespace TrenchBroom
