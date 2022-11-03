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

#include "IO/SourceMdlParser.h"

#include "Assets/EntityModel.h"
#include "Ensure.h"
#include "Exceptions.h"
#include "IO/File.h"
#include "IO/Path.h"
#include "IO/ResourceUtils.h"
#include "IO/SourceMdlDataAccessor.h"
#include "IO/SourceMdlHelpers.h"
#include "IO/SourceMdlLayout.h"
#include "IO/SourceVvdDataAccessor.h"
#include "IO/SourceVvdLayout.h"
#include "IO/TextureReader.h"
#include "IO/VmtTextureReader.h"
#include "Logger.h"
#include "Model/GameFileSystem.h"
#include "Renderer/IndexRangeMapBuilder.h"
#include "Renderer/PrimType.h"
#include "kdl/string_utils.h"
#include "vecmath/forward.h"
#include "vecmath/mat.h"
#include "vecmath/scalar.h"
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace TrenchBroom {
namespace IO {
static inline IO::Path makeTextureDiskPath(const std::string& dir, const std::string& name) {
  return IO::Path("materials/" + dir + name + ".vmt");
}

std::shared_ptr<File> SourceMdlParser::TextureLookupData::openTextureFile(
  const Model::GameFileSystem& fs, size_t index) const {
  if (index >= texturePaths.size()) {
    throw AssetException("Index was out of range");
  }

  for (const std::string& dir : textureDirs) {
    IO::Path path(makeTextureDiskPath(dir, texturePaths[index]));
    if (fs.fileExists(path)) {
      return fs.openFile(path);
    }
  }

  return std::shared_ptr<File>();
}

SourceMdlParser::SourceMdlParser(
  const IO::Path& path, const std::string& name, const char* begin, const char* end,
  const Model::GameFileSystem& fs)
  : m_path(path)
  , m_name(name)
  , m_begin(begin)
  , m_end(end)
  , m_fs(fs) {
  assert(m_begin < m_end);
  unused(m_end);

  m_mdlData = std::make_unique<SourceMdlDataAccessor>(m_begin, m_end);
}

std::unique_ptr<Assets::EntityModel> SourceMdlParser::doInitializeModel(Logger& logger) {
  const size_t frameCount = calculateNumAnimationFrames();

  std::unique_ptr<Assets::EntityModel> model = std::make_unique<Assets::EntityModel>(
    m_name, Assets::PitchType::MdlInverted, Assets::Orientation::Oriented);

  for (size_t index = 0; index < frameCount; ++index) {
    model->addFrame();
  }

  const TextureLookupData texData = getTexturePaths();

  for (size_t skinrefIndex = 0; skinrefIndex < static_cast<size_t>(m_mdlData->header().numskinref);
       ++skinrefIndex) {
    Assets::EntityModelSurface& surface =
      model->addSurface("skinref_" + std::to_string(skinrefIndex));
    readTextures(logger, texData, skinrefIndex, surface);
  }

  return model;
}

void SourceMdlParser::doLoadFrame(
  size_t frameIndex, Assets::EntityModel& model, Logger& /* logger */) {
  const size_t numFrames = calculateNumAnimationFrames();

  if (frameIndex >= numFrames) {
    throw AssetException(
      "Frame index " + std::to_string(frameIndex) + " was outside frame count " +
      std::to_string(numFrames));
  }

  createEntityModel(model);
}

std::shared_ptr<File> SourceMdlParser::openVtxFile() {
  const IO::Path path = m_path.replaceBasename(m_path.basename() + ".dx90").replaceExtension("vtx");
  return m_fs.openFile(path);
}

std::shared_ptr<File> SourceMdlParser::openVvdFile() {
  const IO::Path path = m_path.replaceExtension("vvd");
  return m_fs.openFile(path);
}

size_t SourceMdlParser::calculateNumAnimationFrames() {
  // TODO: Fix this when we actually load proper animation data
  return 1;
}

void SourceMdlParser::readTextures(
  Logger& logger, const TextureLookupData& texData, size_t skinrefIndex,
  Assets::EntityModelSurface& surface) {
  std::vector<size_t> textureIndices = m_mdlData->readTextureIndicesForSkinRef(skinrefIndex);

  if (textureIndices.empty()) {
    // Should never happen
    throw AssetException("Could not fetch texture indices");
  }

  std::vector<Assets::Texture> textures;

  for (size_t skin = 0; skin < textureIndices.size(); ++skin) {
    const size_t texturePathIndex = textureIndices[skin];
    const std::string textureName =
      "skin_" + std::to_string(skinrefIndex) + "_" + std::to_string(skin);

    try {
      textures.emplace_back(
        loadTexture(logger, texData.openTextureFile(m_fs, texturePathIndex), textureName));
    } catch (const AssetException&) {
      textures.emplace_back(defaultTexture(logger, textureName));
    }
  }

  surface.setSkins(std::move(textures));
}

SourceMdlParser::TextureLookupData SourceMdlParser::getTexturePaths() const {
  TextureLookupData data;
  m_mdlData->readTexturePaths(data.textureDirs, data.texturePaths);
  return data;
}

Assets::Texture SourceMdlParser::loadTexture(
  Logger& logger, std::shared_ptr<IO::File> file, const std::string& textureName) {
  try {
    VmtTextureReader reader(TextureReader::StaticNameStrategy(textureName), m_fs, logger);
    return reader.readTexture(file);
  } catch (const AssetException& ex) {
    logger.warn() << ex.what();
  }

  return defaultTexture(logger, textureName);
}

Assets::Texture SourceMdlParser::defaultTexture(Logger& logger, const std::string& textureName) {
  return loadDefaultTexture(m_fs, logger, textureName);
}

void SourceMdlParser::createEntityModel(Assets::EntityModel& model) {
  SourceVtxDataAccessor vtxAccessor(openVtxFile());
  vtxAccessor.validate(m_mdlData->header());

  SourceVvdDataAccessor vvdAccessor(openVvdFile());
  vvdAccessor.validate(m_mdlData->header());

  ModelData modelData = extractModelData();

  std::vector<vm::mat4x4f> worldToRefPoseMatrices;
  std::vector<vm::mat4x4f> animPoseToWorldMatrices;
  computeBoneMatrices(modelData, worldToRefPoseMatrices, animPoseToWorldMatrices);

  const size_t rootLOD = static_cast<size_t>(m_mdlData->header().rootLOD);
  vm::bbox3f::builder bounds;

  std::map<int32_t, std::vector<Assets::EntityModelVertex>> skinToVerticesMap;

  // Run through each body part. For each body part, we have one submodel.
  // For each submodel, we have N meshes, which should be consecutive in the meshes array.
  // For each mesh that applies in the meshes array, compute the indices from the VTX file.
  for (size_t bodyPartIndex = 0; bodyPartIndex < 1; ++bodyPartIndex) {
    // Body part and submodel vectors should correspond one-to-one.
    ensure(
      bodyPartIndex < modelData.submodels.size(), "Body part did not have corresponding submodel");

    // Get the corresponding submodel.
    const SourceMdlDataAccessor::BodyPartItem<SourceMdlLayout::Submodel>& submodel =
      modelData.submodels[bodyPartIndex];

    // We fetch a list of vertices that correspond to this submodel.
    const std::vector<SourceVvdLayout::Vertex>& vertices = vvdAccessor.consolidateVertices(rootLOD);

    bool foundMatchingMesh = false;
    for (const SourceMdlDataAccessor::BodyPartItem<SourceMdlLayout::Mesh>& mesh :
         modelData.meshes) {
      if (mesh.parentBodyPart != submodel.parentBodyPart) {
        if (!foundMatchingMesh) {
          // Still attempting to find the first mesh.
          continue;
        } else {
          // We found a matching mesh and now have reached the end of that set of meshes.
          break;
        }
      }

      foundMatchingMesh = true;

      std::vector<SourceVtxDataAccessor::IndexList> indexLists =
        vtxAccessor.computeMdlVertexIndices(
          bodyPartIndex, submodel.indexInParent, rootLOD, mesh.indexInParent);

      if (skinToVerticesMap.find(mesh.item.material) == skinToVerticesMap.end()) {
        skinToVerticesMap.insert({mesh.item.material, std::vector<Assets::EntityModelVertex>()});
      }

      for (const SourceVtxDataAccessor::IndexList& list : indexLists) {
        const vm::bbox3f listBounds = generateVertices(
          worldToRefPoseMatrices, animPoseToWorldMatrices, list, vertices,
          static_cast<size_t>(submodel.item.vertexindex + mesh.item.vertexoffset),
          skinToVerticesMap[mesh.item.material]);

        bounds.add(listBounds);
      }
    }
  }

  auto& frame = model.loadFrame(0, "frame_0", bounds.bounds());

  size_t surfaceIndex = 0;
  for (auto& pair : skinToVerticesMap) {
    const auto rangeMap =
      Renderer::IndexRangeMap(Renderer::PrimType::Triangles, 0, pair.second.size());
    model.surface(surfaceIndex).addIndexedMesh(frame, std::move(pair.second), std::move(rangeMap));
    ++surfaceIndex;
  }
}

SourceMdlParser::ModelData SourceMdlParser::extractModelData() const {
  ModelData data;

  // TODO: Choose body index? Are we provided with the body index?
  data.textures = getTexturePaths();
  data.bones = m_mdlData->readBones();
  data.bodyParts = m_mdlData->readBodyParts();
  data.submodels = m_mdlData->readSubmodels(data.bodyParts, 0);
  data.meshes = m_mdlData->readMeshes(data.submodels);

  return data;
}

vm::bbox3f SourceMdlParser::generateVertices(
  const std::vector<vm::mat4x4f>& worldToRefPoseMatrices,
  const std::vector<vm::mat4x4f>& animPoseToWorldMatrices,
  const SourceVtxDataAccessor::IndexList& indices,
  const std::vector<SourceVvdLayout::Vertex>& inVertices, size_t beginVertex,
  std::vector<Assets::EntityModelVertex>& outVertices) {
  ensure(
    worldToRefPoseMatrices.size() == animPoseToWorldMatrices.size(),
    "Expected bone matrix vectors to be same size");

  vm::bbox3f::builder bounds;

  if (!indices.isTriStrip) {
    for (uint16_t index : indices.indices) {
      // Convert to index within all the submodel's vertices
      const size_t vertexIndex = static_cast<size_t>(index) + beginVertex;

      // TODO: Handle this more gracefully
      if (vertexIndex >= inVertices.size()) {
        throw AssetException(
          "Encountered out-of-range index " + std::to_string(vertexIndex) +
          " when vertex buffer size is " + std::to_string(inVertices.size()));
      }

      const SourceVvdLayout::Vertex& vertex = inVertices[vertexIndex];
      const vm::vec2f texCoOrd(vertex.texCoOrd[0], vertex.texCoOrd[1]);
      vm::vec3f position(vertex.position[0], vertex.position[1], vertex.position[2]);

      const SourceVvdLayout::BoneWeights& boneWeights = vertex.boneWeights;

      if (boneWeights.numbones > 0) {
        vm::vec3f weightedPosition;

        for (size_t weightIndex = 0;
             weightIndex < boneWeights.numbones && weightIndex < ArraySize(boneWeights.weight);
             ++weightIndex) {
          const size_t boneIndex = static_cast<size_t>(boneWeights.bone[weightIndex]);
          weightedPosition =
            weightedPosition +
            (boneWeights.weight[weightIndex] *
             (position * worldToRefPoseMatrices[boneIndex] * animPoseToWorldMatrices[boneIndex]));
        }

        position = weightedPosition;
      }

      outVertices.emplace_back(position, texCoOrd);
      bounds.add(position);
    }
  } else {
    throw AssetException("TODO: Add support for generating vertices from triangle strips");
  }

  return bounds.bounds();
}

std::vector<vm::mat4x4f> SourceMdlParser::computeBoneMatrices(const ModelData& data) {
  std::vector<vm::mat4x4f> localBoneMatrices;
  localBoneMatrices.reserve(data.bones.size());

  const SourceMdlDataAccessor::MetaItem<SourceMdlLayout::AnimationDescription> animDesc =
    m_mdlData->readLocalAnimationDescription(0);

  // First of all, build up the local transforms for each bone.
  for (const SourceMdlDataAccessor::MetaItem<SourceMdlLayout::Bone>& bone : data.bones) {
    size_t animOffset =
      static_cast<size_t>(static_cast<int32_t>(animDesc.fileOffset) + animDesc.item.animindex);

    SourceMdlDataAccessor::MetaItem<SourceMdlLayout::Animation> anim =
      m_mdlData->readLocalAnimation(animOffset);

    while (static_cast<size_t>(anim.item.bone) != bone.indexInParent) {
      animOffset = calculateNextAnimationOffset(anim.item, anim.fileOffset);

      if (animOffset < 1) {
        break;
      }

      anim = m_mdlData->readLocalAnimation(animOffset);
    }

    vm::mat4x4f boneMatrix;

    if (animOffset != 0) {
      boneMatrix = m_mdlData->computeBoneToParentMatrix(bone.item, anim, 0);
    } else if (animDesc.item.flags & SourceMdlLayout::ANIMFLAG_DELTA) {
      boneMatrix = vm::mat4x4f::identity();
    } else {
      boneMatrix = matrixFromRotAndPos(
        mdlArrayToQuat(bone.item.quat),
        vm::vec3f(bone.item.pos[0], bone.item.pos[1], bone.item.pos[2]));
    }

    localBoneMatrices.emplace_back(boneMatrix);
  }

  // Next, generate a world transform for each bone, where each transform accounts for the
  // transforms of its parents.
  std::vector<vm::mat4x4f> worldBoneMatrices =
    computeBoneToWorldMatrices(stripMetaItems(data.bones));

  ensure(
    worldBoneMatrices.size() == localBoneMatrices.size(),
    "Expected both bone matrix vectors to be the same size");

  // Vertices are provided in world space. To compute a final vertex position, we need to transform
  // the vertex to bone space, apply the local bone transformation computed from the animation
  // frame, and then transform back to world space. Compute a matrix to do this for each bone.
  for (size_t index = 0; index < worldBoneMatrices.size(); ++index) {
    vm::mat4x4f& worldMatrix = worldBoneMatrices[index];
    const vm::mat4x4f& localMatrix = localBoneMatrices[index];
    auto [invertible, inverseWorldMatrix] = vm::invert(worldMatrix);
    ensure(invertible, "Expected matrix to be invertible");

    worldMatrix = inverseWorldMatrix * localMatrix * worldMatrix;
  }

  return worldBoneMatrices;
}

void SourceMdlParser::computeBoneMatrices(
  const ModelData& data, std::vector<vm::mat4x4f>& worldToRefPoseMatrices,
  std::vector<vm::mat4x4f>& animPoseToWorldMatrices) {
  const std::vector<SourceMdlLayout::Bone> strippedBones = stripMetaItems(data.bones);

  // Firstly, compute matrices that take world positions to bone space, with the bones in the
  // reference pose.
  worldToRefPoseMatrices = computeBoneToWorldMatrices(strippedBones);
  invertMatrices(worldToRefPoseMatrices);

  // Next, compute matrices that take a bone posed by animation and convert that position to world
  // space.
  animPoseToWorldMatrices.resize(worldToRefPoseMatrices.size());

  const SourceMdlDataAccessor::MetaItem<SourceMdlLayout::AnimationDescription> animDesc =
    m_mdlData->readLocalAnimationDescription(0);

  for (size_t index = 0; index < data.bones.size(); ++index) {
    const SourceMdlDataAccessor::MetaItem<SourceMdlLayout::Bone>& bone = data.bones[index];
    size_t animOffset =
      static_cast<size_t>(static_cast<int32_t>(animDesc.fileOffset) + animDesc.item.animindex);

    SourceMdlDataAccessor::MetaItem<SourceMdlLayout::Animation> anim =
      m_mdlData->readLocalAnimation(animOffset);

    while (static_cast<size_t>(anim.item.bone) != bone.indexInParent) {
      animOffset = calculateNextAnimationOffset(anim.item, anim.fileOffset);

      if (animOffset < 1) {
        break;
      }

      anim = m_mdlData->readLocalAnimation(animOffset);
    }

    vm::mat4x4f boneMatrix = vm::mat4x4f::identity();

    if (animOffset != 0) {
      boneMatrix = m_mdlData->computeBoneToParentMatrix(bone.item, anim, 0);
    } else if (!(animDesc.item.flags & SourceMdlLayout::ANIMFLAG_DELTA)) {
      boneMatrix = matrixFromRotAndPos(
        mdlArrayToQuat(bone.item.quat),
        vm::vec3f(bone.item.pos[0], bone.item.pos[1], bone.item.pos[2]));
    }

    const int32_t parentIndex = bone.item.parent;

    if (parentIndex >= 0 && static_cast<size_t>(parentIndex) < animPoseToWorldMatrices.size()) {
      const vm::mat4x4f& parentMatrix = animPoseToWorldMatrices[static_cast<size_t>(parentIndex)];
      boneMatrix = boneMatrix * parentMatrix;
    } else {
      boneMatrix = boneMatrix * STUDIOMDL_ROOT_AXIS_TRANSFORM;
    }

    animPoseToWorldMatrices[index] = boneMatrix;
  }
}
} // namespace IO
} // namespace TrenchBroom
