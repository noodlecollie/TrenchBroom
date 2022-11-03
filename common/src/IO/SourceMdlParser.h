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

#include "Assets/EntityModel_Forward.h"
#include "Assets/Texture.h"
#include "IO/EntityModelParser.h"
#include "IO/Reader.h"
#include "IO/SourceMdlDataAccessor.h"
#include "IO/SourceMdlLayout.h"
#include "IO/SourceVtxDataAccessor.h"
#include "IO/SourceVvdLayout.h"
#include "Model/GameFileSystem.h"
#include <memory>
#include <string>
#include <vector>

namespace TrenchBroom {
namespace Model {
class GameFileSystem;
}

namespace IO {
class Reader;
class File;

class SourceMdlParser : public EntityModelParser {
public:
  SourceMdlParser(
    const IO::Path& path, const std::string& name, const char* begin, const char* end,
    const Model::GameFileSystem& fs);

private:
  struct TextureLookupData {
    std::vector<std::string> textureDirs;
    std::vector<std::string> texturePaths;

    std::shared_ptr<File> openTextureFile(const Model::GameFileSystem& fs, size_t index) const;
  };

  struct ModelData {
    TextureLookupData textures;
    std::vector<SourceMdlDataAccessor::MetaItem<SourceMdlLayout::Bone>> bones;
    std::vector<SourceMdlDataAccessor::MetaItem<SourceMdlLayout::BodyPart>> bodyParts;
    std::vector<SourceMdlDataAccessor::BodyPartItem<SourceMdlLayout::Submodel>> submodels;
    std::vector<SourceMdlDataAccessor::BodyPartItem<SourceMdlLayout::Mesh>> meshes;
  };

  std::unique_ptr<Assets::EntityModel> doInitializeModel(Logger& logger) override;
  void doLoadFrame(size_t frameIndex, Assets::EntityModel& model, Logger& logger) override;

  std::shared_ptr<File> openVtxFile();
  std::shared_ptr<File> openVvdFile();

  size_t calculateNumAnimationFrames();
  void readTextures(
    Logger& logger, const TextureLookupData& texData, size_t skinrefIndex,
    Assets::EntityModelSurface& surface);
  TextureLookupData getTexturePaths() const;
  Assets::Texture loadTexture(
    Logger& logger, std::shared_ptr<IO::File> file, const std::string& textureName);
  Assets::Texture defaultTexture(Logger& logger, const std::string& textureName);
  void createEntityModel(Assets::EntityModel& model);
  ModelData extractModelData() const;
  vm::bbox3f generateVertices(
    const std::vector<vm::mat4x4f>& worldToRefPoseMatrices,
    const std::vector<vm::mat4x4f>& animPoseToWorldMatrices,
    const SourceVtxDataAccessor::IndexList& indices,
    const std::vector<SourceVvdLayout::Vertex>& inVertices, size_t beginVertex,
    std::vector<Assets::EntityModelVertex>& outVertices);
  std::vector<vm::mat4x4f> computeBoneMatrices(const ModelData& data);

  void computeBoneMatrices(
    const ModelData& data, std::vector<vm::mat4x4f>& worldToRefPoseMatrices,
    std::vector<vm::mat4x4f>& animPoseToWorldMatrices);

  IO::Path m_path;
  std::string m_name;
  const char* m_begin;
  const char* m_end;
  const Model::GameFileSystem& m_fs;
  std::unique_ptr<SourceMdlDataAccessor> m_mdlData;
};
} // namespace IO
} // namespace TrenchBroom
