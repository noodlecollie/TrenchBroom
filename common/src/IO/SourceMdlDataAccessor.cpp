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

#include "IO/SourceMdlDataAccessor.h"
#include "Exceptions.h"
#include "IO/ReaderException.h"
#include "IO/SourceMdlFormatUtils.h"
#include "IO/SourceMdlHelpers.h"
#include "IO/SourceMdlLayout.h"
#include "kdl/string_utils.h"
#include "vecmath/forward.h"
#include "vecmath/mat.h"
#include "vecmath/mat_ext.h"
#include "vecmath/quat.h"
#include <string>

namespace TrenchBroom {
namespace IO {
SourceMdlDataAccessor::SourceMdlDataAccessor(const char* begin, const char* end)
  : m_reader(Reader::from(begin, end).buffer()) {
  m_reader.read(reinterpret_cast<char*>(&m_header), sizeof(m_header));
  validate();
}

const SourceMdlLayout::Header& SourceMdlDataAccessor::header() const {
  return m_header;
}

std::string SourceMdlDataAccessor::readString(size_t offset) {
  if (offset >= m_reader.size()) {
    throw ReaderException(
      "String offset " + std::to_string(offset) + " exceeded data length of " +
      std::to_string(m_reader.size()) + " bytes");
  }

  const char* begin = m_reader.begin() + offset;
  const char* bufferEnd = m_reader.end();

  for (const char* cursor = begin; cursor < bufferEnd; ++cursor) {
    if (!(*cursor)) {
      return std::string(begin);
    }
  }

  throw ReaderException("String at offset " + std::to_string(offset) + " was unterminated");
}

void SourceMdlDataAccessor::readTexturePaths(
  std::vector<std::string>& dirs, std::vector<std::string>& paths) {
  dirs.clear();
  paths.clear();

  m_reader.seekFromBegin(static_cast<size_t>(m_header.cdtextureindex));

  for (int32_t index = 0; index < m_header.numcdtextures; ++index) {
    const size_t nameIndex = m_reader.readSize<int32_t>();
    const size_t pos = m_reader.position();
    dirs.emplace_back(readString(nameIndex));
    m_reader.seekFromBegin(pos);
  }

  m_reader.seekFromBegin(static_cast<size_t>(m_header.textureindex));

  for (int32_t index = 0; index < m_header.numtextures; ++index) {
    const size_t offset = m_reader.position();

    SourceMdlLayout::Texture textureItem{};
    m_reader.read(reinterpret_cast<char*>(&textureItem), sizeof(SourceMdlLayout::Texture));

    const size_t pos = m_reader.position();
    paths.emplace_back(readString(offset + static_cast<size_t>(textureItem.sznameindex)));
    m_reader.seekFromBegin(pos);
  }

  if (dirs.empty()) {
    // Just add the root.
    dirs.emplace_back("");
  }

  // Often the directory separators are Windows slashes but the texture name separators are normal
  // slashes. Convert everything to be consistent.

  for (std::string& str : dirs) {
    kdl::str_replace_every(str, "\\", "/");
  }

  for (std::string& str : paths) {
    kdl::str_replace_every(str, "\\", "/");
  }
}

std::vector<size_t> SourceMdlDataAccessor::readTextureIndicesForSkinRef(size_t skinRefIndex) {
  const size_t numSkinFamilies = static_cast<size_t>(m_header.numskinfamilies);
  const size_t numSkinRefs = static_cast<size_t>(m_header.numskinref);

  std::vector<size_t> out;
  out.reserve(numSkinFamilies);

  for (size_t skinFamily = 0; skinFamily < numSkinFamilies; ++skinFamily) {
    size_t offset = static_cast<size_t>(m_header.skinindex) +
                    (skinFamily * numSkinRefs * sizeof(SourceMdlLayout::SkinRef));

    offset += skinRefIndex * sizeof(SourceMdlLayout::SkinRef);
    m_reader.seekFromBegin(offset);

    SourceMdlLayout::SkinRef skinRefData{};
    m_reader.read(reinterpret_cast<char*>(&skinRefData), sizeof(skinRefData));

    out.emplace_back(static_cast<size_t>(skinRefData.index));
  }

  return out;
}

std::vector<SourceMdlDataAccessor::MetaItem<SourceMdlLayout::Bone>> SourceMdlDataAccessor::
  readBones() {
  return extractItems<SourceMdlLayout::Bone>(m_header.boneindex, m_header.numbones);
}

std::vector<SourceMdlDataAccessor::MetaItem<SourceMdlLayout::BodyPart>> SourceMdlDataAccessor::
  readBodyParts() {
  return extractItems<SourceMdlLayout::BodyPart>(m_header.bodypartindex, m_header.numbodyparts);
}

std::vector<SourceMdlDataAccessor::BodyPartItem<SourceMdlLayout::Submodel>> SourceMdlDataAccessor::
  readSubmodels(
    const std::vector<SourceMdlDataAccessor::MetaItem<SourceMdlLayout::BodyPart>>& bodyParts,
    size_t bodyIndex) {
  const size_t numBodyParts = bodyParts.size();

  if (numBodyParts != static_cast<size_t>(m_header.numbodyparts)) {
    throw ReaderException(
      "Provided vector of " + std::to_string(numBodyParts) +
      " body parts does not match header count of " + std::to_string(m_header.numbodyparts) +
      " body parts");
  }

  // One submodel selected per body part.
  std::vector<BodyPartItem<SourceMdlLayout::Submodel>> submodels;
  submodels.resize(numBodyParts);

  for (size_t index = 0; index < numBodyParts; ++index) {
    const SourceMdlLayout::BodyPart& bodyPart = bodyParts[index].item;
    const size_t submodelIndex = calculateSubmodelIndex(bodyPart, bodyIndex);

    // According to the SDK code, the submodels array is found at bodyPart.submodelindex bytes from
    // the beginning of the body part structure. We then need to offset into that array by
    // submodelIndex * sizeof(Submodel).
    const size_t submodelsArrayFileOffset = static_cast<size_t>(
      static_cast<int32_t>(bodyParts[index].fileOffset) + bodyPart.submodelindex);
    const size_t submodelFileOffset =
      submodelsArrayFileOffset + (submodelIndex * sizeof(SourceMdlLayout::Submodel));

    BodyPartItem<SourceMdlLayout::Submodel>& submodel = submodels[index];
    submodel.fileOffset = submodelFileOffset;
    submodel.indexInParent = submodelIndex;
    submodel.parentBodyPart = index;

    m_reader.seekFromBegin(submodel.fileOffset);
    m_reader.read(reinterpret_cast<char*>(&submodel.item), sizeof(submodel.item));
  }

  return submodels;
}

std::vector<SourceMdlDataAccessor::BodyPartItem<SourceMdlLayout::Submodel>> SourceMdlDataAccessor::
  readSubmodels(size_t bodyIndex) {
  return readSubmodels(readBodyParts(), bodyIndex);
}

std::vector<SourceMdlDataAccessor::BodyPartItem<SourceMdlLayout::Mesh>> SourceMdlDataAccessor::
  readMeshes(
    const std::vector<SourceMdlDataAccessor::BodyPartItem<SourceMdlLayout::Submodel>>& submodels) {
  std::vector<BodyPartItem<SourceMdlLayout::Mesh>> meshes;

  for (size_t index = 0; index < submodels.size(); ++index) {
    const BodyPartItem<SourceMdlLayout::Submodel>& submodel = submodels[index];
    const size_t& submodelFileOffset = submodel.fileOffset;

    const size_t origMeshesSize = meshes.size();
    meshes.resize(origMeshesSize + static_cast<size_t>(submodel.item.nummeshes));

    for (size_t meshIndex = 0; meshIndex < static_cast<size_t>(submodel.item.nummeshes);
         ++meshIndex) {
      // According to the SDK code, the meshes array is found at submodel.meshindex bytes from the
      // beginning of the submodel structure. We then need to offset into that array by meshIndex *
      // sizeof(Mesh).
      const size_t meshArrayFileOffset =
        static_cast<size_t>(static_cast<int32_t>(submodelFileOffset) + submodel.item.meshindex);
      const size_t meshFileOffset =
        meshArrayFileOffset + (meshIndex * sizeof(SourceMdlLayout::Mesh));

      BodyPartItem<SourceMdlLayout::Mesh>& mesh = meshes[origMeshesSize + meshIndex];
      mesh.parentBodyPart = submodel.parentBodyPart;
      mesh.indexInParent = meshIndex;
      mesh.fileOffset = meshFileOffset;

      m_reader.seekFromBegin(mesh.fileOffset);
      m_reader.read(reinterpret_cast<char*>(&mesh.item), sizeof(mesh.item));
    }
  }

  return meshes;
}

SourceMdlDataAccessor::MetaItem<SourceMdlLayout::AnimationDescription> SourceMdlDataAccessor::
  readLocalAnimationDescription(size_t index) {
  if (index >= static_cast<size_t>(m_header.numlocalanim)) {
    throw ReaderException(
      "Animation description index " + std::to_string(index) + " was out of range");
  }

  const size_t animDescOffset =
    static_cast<size_t>(m_header.localanimindex) + (index * sizeof(SourceMdlLayout::Animation));

  MetaItem<SourceMdlLayout::AnimationDescription> animationDescription{};
  animationDescription.indexInParent = index;
  animationDescription.fileOffset = animDescOffset;

  m_reader.seekFromBegin(animationDescription.fileOffset);
  m_reader.read(
    reinterpret_cast<char*>(&animationDescription.item), sizeof(animationDescription.item));

  return animationDescription;
}

SourceMdlDataAccessor::MetaItem<SourceMdlLayout::Animation> SourceMdlDataAccessor::
  readLocalAnimation(size_t offset) {
  SourceMdlDataAccessor::MetaItem<SourceMdlLayout::Animation> animation{};
  animation.fileOffset = offset;
  animation.indexInParent = 0;

  m_reader.seekFromBegin(animation.fileOffset);
  m_reader.read(reinterpret_cast<char*>(&animation.item), sizeof(animation.item));

  return animation;
}

vm::quatf SourceMdlDataAccessor::readAnimationRotation(
  const MetaItem<SourceMdlLayout::Animation>& animation, size_t frameIndex,
  const vm::quatf& defaultBoneRot, const vm::vec3f& baseEulerRot,
  const vm::vec3f& baseEulerRotScale, const vm::quatf& alignRot, bool boneHasFixedAlignment) {
  if (animation.item.flags & SourceMdlLayout::ANIMFLAG_ROTATION_IS_QUAT48) {
    return convertQuaternion48(readAnimationData<SourceMdlLayout::Quaternion48>(animation));
  }

  if (animation.item.flags & SourceMdlLayout::ANIMFLAG_ROTATION_IS_QUAT64) {
    return convertQuaternion64(readAnimationData<SourceMdlLayout::Quaternion64>(animation));
  }

  if (!(animation.item.flags & SourceMdlLayout::ANIMFLAG_ROTATION_IS_VALUEPTR)) {
    if (animation.item.flags & SourceMdlLayout::ANIMFLAG_DELTA) {
      return vm::quatf(1.0f, vm::vec3f(0.0f, 0.0f, 0.0f));
    } else {
      return defaultBoneRot;
    }
  }

  return readAnimationRotationFromValues(
    animation, frameIndex, baseEulerRot, baseEulerRotScale, alignRot, boneHasFixedAlignment);
}

vm::vec3f SourceMdlDataAccessor::readAnimationPosition(
  const MetaItem<SourceMdlLayout::Animation>& animation, size_t frameIndex,
  const vm::vec3f& basePos, const vm::vec3f& basePosScale) {
  if (animation.item.flags & SourceMdlLayout::ANIMFLAG_POSITION_IS_VEC48) {
    return convertVector48(readAnimationPosDataVector48(animation));
  }

  if (!(animation.item.flags & SourceMdlLayout::ANIMFLAG_POSITION_IS_VALUEPTR)) {
    if (animation.item.flags & SourceMdlLayout::ANIMFLAG_DELTA) {
      return vm::vec3f();
    } else {
      return basePos;
    }
  }

  return readAnimationPositionFromValues(animation, frameIndex, basePos, basePosScale);
}

vm::mat4x4f SourceMdlDataAccessor::computeBoneToParentMatrix(
  const SourceMdlLayout::Bone& bone, const MetaItem<SourceMdlLayout::Animation>& animation,
  size_t animFrame) {
  const vm::vec3f bonePos(bone.pos[0], bone.pos[1], bone.pos[2]);
  const vm::vec3f bonePosScale(bone.posscale[0], bone.posscale[1], bone.posscale[2]);
  const vm::quat boneQuat = mdlArrayToQuat(bone.quat);
  const vm::vec3f boneEulerRot(bone.rot[0], bone.rot[1], bone.rot[2]);
  const vm::vec3f boneEulerRotScale(bone.rotscale[0], bone.rotscale[1], bone.rotscale[2]);
  const vm::quatf boneAlignQuat = mdlArrayToQuat(bone.qAlignment);

  const vm::vec3f animPos = readAnimationPosition(animation, animFrame, bonePos, bonePosScale);
  const vm::quatf animRot = readAnimationRotation(
    animation, animFrame, boneQuat, boneEulerRot, boneEulerRotScale, boneAlignQuat,
    bone.flags & SourceMdlLayout::BONEFLAG_FIXED_ALIGNMENT);

  return matrixFromRotAndPos(animRot, animPos);
}

void SourceMdlDataAccessor::validate() const {
  if (!isSourceMdlIdentifier(m_header.id)) {
    throw FileFormatException("Unknown Source MDL ident: " + std::to_string(m_header.id));
  }

  if (!isSourceMdlVersion(m_header.version)) {
    throw FileFormatException(
      "Unsupported Source MDL version: " + std::to_string(m_header.version));
  }
}

vm::quatf SourceMdlDataAccessor::readAnimationRotationFromValues(
  const MetaItem<SourceMdlLayout::Animation>& animation, size_t frameIndex,
  const vm::vec3f& baseEulerRot, const vm::vec3f& baseEulerRotScale, const vm::quatf& alignRot,
  bool boneHasFixedAlignment) {
  const MetaItem<SourceMdlLayout::AnimationValuePtr> valuePtr =
    readAnimationDataMeta<SourceMdlLayout::AnimationValuePtr>(animation);

  size_t valueOffsets[3];
  valueOffsets[0] = calculateLocalAnimationValueOffset(valuePtr, 0);
  valueOffsets[1] = calculateLocalAnimationValueOffset(valuePtr, 1);
  valueOffsets[2] = calculateLocalAnimationValueOffset(valuePtr, 2);

  vm::vec3f eulerAngles;

  for (size_t arrayIndex = 0; arrayIndex < 3; ++arrayIndex) {
    eulerAngles[arrayIndex] =
      extractAnimationValue(valueOffsets[arrayIndex], frameIndex, baseEulerRotScale[arrayIndex]);
  }

  if (!(animation.item.flags & SourceMdlLayout::ANIMFLAG_DELTA)) {
    eulerAngles[0] += baseEulerRot[0];
    eulerAngles[1] += baseEulerRot[1];
    eulerAngles[2] += baseEulerRot[2];
  }

  vm::quatf outQuat = eulerAnglesToQuaternion(eulerAngles);

  if (!(animation.item.flags & SourceMdlLayout::ANIMFLAG_DELTA) && boneHasFixedAlignment) {
    outQuat = alignQuaternion(outQuat, alignRot);
  }

  return outQuat;
}

vm::vec3f SourceMdlDataAccessor::readAnimationPositionFromValues(
  const MetaItem<SourceMdlLayout::Animation>& animation, size_t frameIndex,
  const vm::vec3f& basePos, const vm::vec3f& basePosScale) {
  const MetaItem<SourceMdlLayout::AnimationValuePtr> valuePtr = readAnimationPosValuePtr(animation);

  size_t valueOffsets[3];
  valueOffsets[0] = calculateLocalAnimationValueOffset(valuePtr, 0);
  valueOffsets[1] = calculateLocalAnimationValueOffset(valuePtr, 1);
  valueOffsets[2] = calculateLocalAnimationValueOffset(valuePtr, 2);

  vm::vec3f outPos;

  for (size_t arrayIndex = 0; arrayIndex < 3; ++arrayIndex) {
    outPos[arrayIndex] =
      extractAnimationValue(valueOffsets[arrayIndex], frameIndex, basePosScale[arrayIndex]);
  }

  if (!(animation.item.flags & SourceMdlLayout::ANIMFLAG_DELTA)) {
    outPos[0] += basePos[0];
    outPos[1] += basePos[1];
    outPos[2] += basePos[2];
  }

  return outPos;
}

size_t SourceMdlDataAccessor::calculateLocalAnimationValueOffset(
  const MetaItem<SourceMdlLayout::AnimationValuePtr>& ptr, size_t index) {
  if (index >= ArraySize(ptr.item.offset)) {
    throw ReaderException(
      "Index " + std::to_string(index) + " was out of range for AnimationValuePtr");
  }

  return static_cast<size_t>(
    static_cast<int32_t>(ptr.fileOffset) + static_cast<int32_t>(ptr.item.offset[index]));
}

float SourceMdlDataAccessor::extractAnimationValue(size_t offset, size_t frame, float scale) {
  size_t index = frame;

  SourceMdlLayout::AnimationValue animValue{};
  m_reader.seekFromBegin(offset);
  m_reader.read(reinterpret_cast<char*>(&animValue), sizeof(animValue));

  while (static_cast<size_t>(animValue.v.num.total) <= index) {
    index -= static_cast<size_t>(animValue.v.num.total);

    const size_t itemIncrement = static_cast<size_t>(animValue.v.num.valid) + 1;
    offset += itemIncrement * sizeof(SourceMdlLayout::AnimationValue);
    m_reader.seekFromBegin(offset);
    m_reader.read(reinterpret_cast<char*>(&animValue), sizeof(animValue));

    if (animValue.v.num.total == 0) {
      // Reached end of animation data stream.
      return 0.0f;
    }
  }

  if (static_cast<size_t>(animValue.v.num.valid) > index) {
    // v1 = panimvalue[k + 1].value * scale;
    offset += (index + 1) * sizeof(SourceMdlLayout::AnimationValue);
  } else {
    // v1 = panimvalue[panimvalue->num.valid].value * scale;
    offset += animValue.v.num.valid * sizeof(SourceMdlLayout::AnimationValue);
  }

  m_reader.seekFromBegin(offset);
  m_reader.read(reinterpret_cast<char*>(&animValue), sizeof(animValue));
  return animValue.v.value * scale;
}

SourceMdlLayout::Vector48 SourceMdlDataAccessor::readAnimationPosDataVector48(
  const MetaItem<SourceMdlLayout::Animation>& animation) {
  size_t offset = animation.fileOffset + sizeof(SourceMdlLayout::Animation);

  if (animation.item.flags & SourceMdlLayout::ANIMFLAG_ROTATION_IS_QUAT48) {
    offset += sizeof(SourceMdlLayout::Quaternion48);
  } else if (animation.item.flags & SourceMdlLayout::ANIMFLAG_ROTATION_IS_QUAT64) {
    offset += sizeof(SourceMdlLayout::Quaternion64);
  }

  SourceMdlLayout::Vector48 data{};
  m_reader.seekFromBegin(offset);
  m_reader.read(reinterpret_cast<char*>(&data), sizeof(data));
  return data;
}

SourceMdlDataAccessor::MetaItem<SourceMdlLayout::AnimationValuePtr> SourceMdlDataAccessor::
  readAnimationPosValuePtr(const MetaItem<SourceMdlLayout::Animation>& animation) {
  size_t offset = animation.fileOffset + sizeof(SourceMdlLayout::Animation);

  if (animation.item.flags & SourceMdlLayout::ANIMFLAG_ROTATION_IS_VALUEPTR) {
    // Skip first valueptr struct, which is for rotation.
    offset += sizeof(SourceMdlLayout::AnimationValuePtr);
  }

  MetaItem<SourceMdlLayout::AnimationValuePtr> data{};
  m_reader.seekFromBegin(offset);
  m_reader.read(reinterpret_cast<char*>(&data.item), sizeof(data.item));
  return data;
}
} // namespace IO
} // namespace TrenchBroom
