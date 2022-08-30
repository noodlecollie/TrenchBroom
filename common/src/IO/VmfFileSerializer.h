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

#include "IO/NodeSerializer.h"

namespace TrenchBroom {
namespace Model {
class BezierPatch;
class Brush;
class BrushNode;
class BrushFace;
class EntityProperty;
class Node;
class PatchNode;
} // namespace Model

namespace IO {
class ValveKeyValuesTree;
class ValveKeyValuesNode;

class VmfFileSerializer : public NodeSerializer {
public:
  explicit VmfFileSerializer(std::ostream& stream);
  ~VmfFileSerializer() override;

private:
  using LineStack = std::vector<size_t>;

  struct PrecomputedString {
    std::string string;
    size_t lineCount;
  };

  struct BrushWithAssignedIDs {
    const Model::BrushNode* brushNode;
    size_t brushID;
    size_t beginFaceID;
  };

  void doBeginFile(const std::vector<const Model::Node*>& nodes) override;
  void doEndFile() override;
  void doBeginEntity(const Model::Node* node) override;
  void doEndEntity(const Model::Node* node) override;
  void doEntityProperty(const Model::EntityProperty& property) override;
  void doBrush(const Model::BrushNode* brushNode) override;
  void doBrushFace(const Model::BrushFace& face) override;
  void doPatch(const Model::PatchNode* patchNode) override;

  void writePreamble();
  void pushStartLine();
  size_t popStartLine();
  void setFilePosition(const Model::Node* node);
  void precomputeBrushesAndPatches(const std::vector<const Model::Node*>& rootNodes);
  PrecomputedString writeBrushFaces(const BrushWithAssignedIDs& brush) const;
  size_t writeBrushFace(std::ostream& stream, const Model::BrushFace& face, size_t faceID) const;

  std::ostream& m_stream;
  size_t m_line = 1;
  LineStack m_startLineStack;

  std::unordered_map<const Model::Node*, PrecomputedString> m_nodeToPrecomputedString;
};
} // namespace IO
} // namespace TrenchBroom
