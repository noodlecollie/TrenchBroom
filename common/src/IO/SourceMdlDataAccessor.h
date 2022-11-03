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

#include "IO/Reader.h"
#include "IO/SourceMdlLayout.h"
#include "vecmath/forward.h"
#include <memory>
#include <string>
#include <vector>

namespace TrenchBroom {
namespace IO {
class SourceMdlDataAccessor {
public:
  template <typename T> struct MetaItem {
    size_t fileOffset;
    size_t indexInParent;
    T item;
  };

  template <typename T> struct BodyPartItem : public MetaItem<T> { size_t parentBodyPart; };

  explicit SourceMdlDataAccessor(const char* begin, const char* end);

  const SourceMdlLayout::Header& header() const;
  std::string readString(size_t offset);
  void readTexturePaths(std::vector<std::string>& dirs, std::vector<std::string>& paths);
  std::vector<size_t> readTextureIndicesForSkinRef(size_t skinRefIndex);
  std::vector<MetaItem<SourceMdlLayout::Bone>> readBones();
  std::vector<MetaItem<SourceMdlLayout::BodyPart>> readBodyParts();
  std::vector<BodyPartItem<SourceMdlLayout::Submodel>> readSubmodels(
    const std::vector<MetaItem<SourceMdlLayout::BodyPart>>& bodyParts, size_t bodyIndex);
  std::vector<BodyPartItem<SourceMdlLayout::Submodel>> readSubmodels(size_t bodyIndex);
  std::vector<BodyPartItem<SourceMdlLayout::Mesh>> readMeshes(
    const std::vector<BodyPartItem<SourceMdlLayout::Submodel>>& submodels);

  MetaItem<SourceMdlLayout::AnimationDescription> readLocalAnimationDescription(size_t index);
  MetaItem<SourceMdlLayout::Animation> readLocalAnimation(size_t offset);
  MetaItem<SourceMdlLayout::AnimationValue> readLocalAnimationValue(
    const MetaItem<SourceMdlLayout::AnimationValuePtr>& ptr, size_t index);

  vm::quatf readAnimationRotation(
    const MetaItem<SourceMdlLayout::Animation>& animation, size_t frameIndex,
    const vm::quatf& defaultBoneRot, const vm::vec3f& baseEulerRot,
    const vm::vec3f& baseEulerRotScale, const vm::quatf& alignRot, bool boneHasFixedAlignment);

  vm::vec3f readAnimationPosition(
    const MetaItem<SourceMdlLayout::Animation>& animation, size_t frameIndex,
    const vm::vec3f& basePos, const vm::vec3f& basePosScale);

  vm::mat4x4f computeBoneToParentMatrix(
    const SourceMdlLayout::Bone& bone, const MetaItem<SourceMdlLayout::Animation>& animation, size_t animFrame);

private:
  template <typename T> inline std::vector<MetaItem<T>> extractItems(size_t offset, size_t count) {
    std::vector<MetaItem<T>> items;

    items.resize(count);
    m_reader.seekFromBegin(offset);

    for (size_t index = 0; index < items.size(); ++index) {
      items[index].indexInParent = index;
      items[index].fileOffset = m_reader.position();
      m_reader.read(reinterpret_cast<char*>(&items[index].item), sizeof(T));
    }

    return items;
  }

  template <typename T>
  inline std::vector<MetaItem<T>> extractItems(int32_t offset, int32_t count) {
    return extractItems<T>(static_cast<size_t>(offset), static_cast<size_t>(count));
  }

  template <typename T> T readAnimationData(const MetaItem<SourceMdlLayout::Animation>& animation) {
    T data{};
    m_reader.seekFromBegin(animation.fileOffset + sizeof(SourceMdlLayout::Animation));
    m_reader.read(reinterpret_cast<char*>(&data), sizeof(data));
    return data;
  }

  template <typename T>
  MetaItem<T> readAnimationDataMeta(const MetaItem<SourceMdlLayout::Animation>& animation) {
    MetaItem<T> data{};
    data.fileOffset = animation.fileOffset + sizeof(SourceMdlLayout::Animation);
    data.indexInParent = 0;

    m_reader.seekFromBegin(data.fileOffset);
    m_reader.read(reinterpret_cast<char*>(&data.item), sizeof(data.item));

    return data;
  }

  void validate() const;
  vm::quatf readAnimationRotationFromValues(
    const MetaItem<SourceMdlLayout::Animation>& animation, size_t frameIndex,
    const vm::vec3f& baseEulerRot, const vm::vec3f& baseEulerRotScale, const vm::quatf& alignRot,
    bool boneHasFixedAlignment);
  vm::vec3f readAnimationPositionFromValues(
    const MetaItem<SourceMdlLayout::Animation>& animation, size_t frameIndex,
    const vm::vec3f& basePos, const vm::vec3f& basePosScale);
  size_t calculateLocalAnimationValueOffset(
    const MetaItem<SourceMdlLayout::AnimationValuePtr>& ptr, size_t index);
  float extractAnimationValue(size_t offset, size_t frame, float scale);
  SourceMdlLayout::Vector48 readAnimationPosDataVector48(
    const MetaItem<SourceMdlLayout::Animation>& animation);
  MetaItem<SourceMdlLayout::AnimationValuePtr> readAnimationPosValuePtr(
    const MetaItem<SourceMdlLayout::Animation>& animation);

  BufferedReader m_reader;
  SourceMdlLayout::Header m_header;
};

template <typename T>
std::vector<T> stripMetaItems(const std::vector<SourceMdlDataAccessor::MetaItem<T>>& in) {
  std::vector<T> out;
  out.reserve(in.size());

  for (const auto& item : in) {
    out.emplace_back(item.item);
  }

  return out;
}
} // namespace IO
} // namespace TrenchBroom
