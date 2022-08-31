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

#include "IO/VmfFileSerializer.h"

#include "Ensure.h"
#include "IO/ValveKeyValuesTree.h"
#include "IO/ValveKeyValuesWriter.h"
#include "IO/ValveKeyValuesWriterUtils.h"
#include "Model/BrushFace.h"
#include "Model/BrushNode.h"
#include "Model/EntityNode.h"
#include "Model/EntityProperties.h"
#include "Model/GroupNode.h"
#include "Model/Layer.h"
#include "Model/LayerNode.h"
#include "Model/Node.h"
#include "Model/PatchNode.h"
#include "Model/WorldNode.h"
#include "kdl/string_compare.h"

#include <fmt/format.h>
#include <iterator> // for std::ostreambuf_iterator
#include <kdl/overload.h>
#include <kdl/parallel.h>
#include <kdl/string_format.h>
#include <kdl/vector_utils.h>
#include <sstream>

namespace TrenchBroom {
namespace IO {
using namespace ValveKeyValuesWriterUtils;

template <typename... ARGS> static inline void format(std::ostream& out, ARGS&&... args) {
  fmt::format_to(std::ostreambuf_iterator<char>(out), std::forward<ARGS>(args)...);
}

template <typename... ARGS>
static inline void addChildFormatted(
  ValveKeyValuesNode* node, const std::string& key, ARGS&&... args) {
  std::stringstream ss;
  fmt::format_to(std::ostreambuf_iterator<char>(ss), std::forward<ARGS>(args)...);
  node->addChildWithValue(key, ss.str());
}

static inline bool isWorldspawn(const Model::Node& node) {
  return node.name() == Model::EntityPropertyValues::WorldspawnClassname;
}

static inline bool isGroupOrLayer(const Model::Node* node) {
  return dynamic_cast<const Model::LayerNode*>(node) || dynamic_cast<const Model::GroupNode*>(node);
}

// If a world brush is part of a layer or group, TrenchBroom makes it appear as if it is part of a
// func_group entity. This messes with how the vbsp will compile the map, so we want to move all of
// these types of brushes to the worldspawn when we write them.
// We should move a brush to be part of the worldspawn if:
// - The brush is not part of a brush entity.
// - The brush is part of a group, or a custom layer that will be exported.
static inline bool shouldMoveBrushToWorldspawn(const Model::BrushNode& brushNode) {
  const Model::EntityNodeBase* entityNode = brushNode.entity();
  const Model::LayerNode* layerNode = brushNode.containingLayer();

  if (!layerNode || !entityNode) {
    return false;
  }

  const Model::Layer& layer = layerNode->layer();
  const Model::Entity& entity = entityNode->entity();

  const bool isInCustomExportedLayer = !layer.defaultLayer() && !layer.omitFromExport();
  const bool isInGroup = brushNode.containedInGroup();
  const bool isWorldBrush = entity.hasProperty(
    Model::EntityPropertyKeys::Classname, Model::EntityPropertyValues::WorldspawnClassname);

  return (isInCustomExportedLayer || isInGroup) && isWorldBrush;
}

// Don't write TrenchBroom-specific keys in a VMF export.
static inline bool shouldExcludeProperty(const std::string& key) {
  return kdl::cs::str_is_prefix(key, "_tb_");
}

VmfFileSerializer::VmfFileSerializer(std::ostream& stream)
  : NodeSerializer()
  , m_stream(stream) {}

VmfFileSerializer::~VmfFileSerializer() {}

void VmfFileSerializer::doBeginFile(const std::vector<const Model::Node*>& rootNodes) {
  writePreamble();
  precomputeBrushesAndPatches(rootNodes);
}

void VmfFileSerializer::doEndFile() {}

void VmfFileSerializer::doBeginEntity(const Model::Node* node) {
  m_processingGroupOrLayer = isGroupOrLayer(node);

  if (m_processingGroupOrLayer) {
    return;
  }

  NodeSerializer::ObjectNo entID = m_entityID++;
  pushStartLine();

  const std::string entKey = isWorldspawn(*node) ? "world" : "entity";
  format(m_stream, "\"{}\"\n", entKey);
  format(m_stream, "{{\n");
  format(m_stream, "\t\"id\" \"{}\"\n", entID);
  m_line += 3;
}

void VmfFileSerializer::doEndEntity(const Model::Node* node) {
  if (m_processingGroupOrLayer) {
    m_processingGroupOrLayer = false;
    return;
  }

  if (isWorldspawn(*node)) {
    writeBrushesMovedToWorldspawn();
  }

  format(m_stream, "}}\n");
  ++m_line;

  setFilePosition(node);
}

void VmfFileSerializer::doEntityProperty(const Model::EntityProperty& attribute) {
  if (m_processingGroupOrLayer || shouldExcludeProperty(attribute.key())) {
    return;
  }

  format(
    m_stream, "\t{} {}\n", quoteEscapedString(attribute.key()),
    quoteEscapedString(attribute.value()));
  ++m_line;
}

void VmfFileSerializer::doBrush(const Model::BrushNode* brush) {
  if (shouldMoveBrushToWorldspawn(*brush)) {
    // Handled in doEndEntity() for worldspawn.
    return;
  }

  pushStartLine();

  auto it = m_nodeToPrecomputedString.find(brush);
  ensure(
    it != std::end(m_nodeToPrecomputedString),
    "Attempted to serialize a brush which was not processed by precomputeBrushesAndPatches()");

  writePrecomputedString(it->first, it->second);
}

void VmfFileSerializer::doBrushFace(const Model::BrushFace& /* face */) {
  ensure(false, "Brush faces are computed in parallel in precomputeBrushesAndPatches(), not here");
}

void VmfFileSerializer::doPatch(const Model::PatchNode* /* patchNode */) {
  // Patch nodes are not currently supported
}

void VmfFileSerializer::writePreamble() {
  // TODO: This is just a carbon copy of what Hammer spat out.
  // We should make proper use of all of this at some point.

  ValveKeyValuesTree tree;
  ValveKeyValuesNode* root = tree.getRoot();

  ValveKeyValuesNode* versionInfo = root->addChild("versioninfo");
  versionInfo->addChildWithValue("editorversion", "400");
  versionInfo->addChildWithValue("editorbuild", "4933");
  versionInfo->addChildWithValue("mapversion", "107");
  versionInfo->addChildWithValue("formatversion", "100");
  versionInfo->addChildWithValue("prefab", "0");

  root->addChild("visgroups");

  ValveKeyValuesNode* viewSettings = root->addChild("viewsettings");
  viewSettings->addChildWithValue("bSnapToGrid", "1");
  viewSettings->addChildWithValue("bShowGrid", "1");
  viewSettings->addChildWithValue("bShowLogicalGrid", "0");
  viewSettings->addChildWithValue("nGridSpacing", "64");
  viewSettings->addChildWithValue("bShow3DGrid", "0");

  ValveKeyValuesNode* cameras = root->addChild("cameras");
  cameras->addChildWithValue("activecamera", "0");

  ValveKeyValuesNode* camera = cameras->addChild("camera");
  camera->addChildWithValue("position", "[0 0 0]");
  camera->addChildWithValue("look", "[64 0 0]");

  ValveKeyValuesNode* cordon = root->addChild("cordon");
  cordon->addChildWithValue("mins", "(-1024 -1024 -1024)");
  cordon->addChildWithValue("mins", "(1024 1024 1024)");
  cordon->addChildWithValue("active", "0");

  ValveKeyValuesWriter(m_stream).write(tree);
  m_line += ValveKeyValuesWriter::countOutputLines(tree);
}

void VmfFileSerializer::pushStartLine() {
  m_startLineStack.push_back(m_line);
}

size_t VmfFileSerializer::popStartLine() {
  assert(!m_startLineStack.empty());
  const size_t result = m_startLineStack.back();
  m_startLineStack.pop_back();
  return result;
}

void VmfFileSerializer::setFilePosition(const Model::Node* node) {
  const size_t start = popStartLine();
  node->setFilePosition(start, m_line - start);
}

void VmfFileSerializer::precomputeBrushesAndPatches(
  const std::vector<const Model::Node*>& rootNodes) {
  std::vector<BrushWithAssignedIDs> nodesToSerialize;
  nodesToSerialize.reserve(rootNodes.size());

  size_t beginFaceID = 1;

  Model::Node::visitAll(
    rootNodes, kdl::overload(
                 [](auto&& thisLambda, const Model::WorldNode* world) {
                   world->visitChildren(thisLambda);
                 },
                 [](auto&& thisLambda, const Model::LayerNode* layer) {
                   layer->visitChildren(thisLambda);
                 },
                 [](auto&& thisLambda, const Model::GroupNode* group) {
                   group->visitChildren(thisLambda);
                 },
                 [](auto&& thisLambda, const Model::EntityNode* entity) {
                   entity->visitChildren(thisLambda);
                 },
                 [&](const Model::BrushNode* brush) {
                   nodesToSerialize.push_back({brush, nodesToSerialize.size() + 1, beginFaceID});
                   beginFaceID += brush->brush().faceCount();
                 },
                 [](const Model::PatchNode*) {
                   // Not currently supported.
                 }));

  // serialize brushes to strings in parallel
  using Entry = std::pair<const Model::Node*, PrecomputedString>;

  std::vector<Entry> result = kdl::vec_parallel_transform(
    std::move(nodesToSerialize), [&](const BrushWithAssignedIDs& brushEntry) {
      return Entry{brushEntry.brushNode, writeBrushFaces(brushEntry)};
    });

  // move strings into a map
  for (auto& entry : result) {
    m_nodeToPrecomputedString.insert(std::move(entry));
  }
}

VmfFileSerializer::PrecomputedString VmfFileSerializer::writeBrushFaces(
  const BrushWithAssignedIDs& brush) const {
  std::stringstream stream;
  size_t lines = 0;

  format(stream, "\t\"solid\"\n");
  format(stream, "\t{{\n");
  format(stream, "\t\t\"id\" \"{}\"\n", brush.brushID);
  lines += 3;

  size_t faceID = brush.beginFaceID;

  for (const Model::BrushFace& face : brush.brushNode->brush().faces()) {
    lines += writeBrushFace(stream, face, faceID++);
  }

  format(stream, "\t}}\n");
  ++lines;

  return PrecomputedString{stream.str(), lines, shouldMoveBrushToWorldspawn(*brush.brushNode)};
}

size_t VmfFileSerializer::writeBrushFace(
  std::ostream& stream, const Model::BrushFace& face, size_t faceID) const {
  // Each face looks something like:
  // side
  // {
  //   "id" "775"
  //   "plane" "(-448 448 0) (1344 448 0) (1344 0 0)"
  //   "material" "TILE/FLOOR_TILEBLUE01"
  //   "uaxis" "[1 0 0 0] 0.125"
  //   "vaxis" "[0 -1 0 0] 0.125"
  //   "rotation" "0"
  //   "lightmapscale" "16"
  //   "smoothing_groups" "0"
  // }

  ValveKeyValuesTree tree;
  ValveKeyValuesNode* side = tree.getRoot()->addChild("side");

  side->addChildWithValue("id", std::to_string(faceID));

  const Model::BrushFace::Points& points = face.points();
  addChildFormatted(
    side, "plane", "({} {} {}) ({} {} {}) ({} {} {})", points[0].x(), points[0].y(), points[0].z(),
    points[1].x(), points[1].y(), points[1].z(), points[2].x(), points[2].y(), points[2].z());

  std::string textureName = face.attributes().textureName().empty()
                              ? Model::BrushFaceAttributes::NoTextureName
                              : face.attributes().textureName();

  if (textureName == Model::BrushFaceAttributes::NoTextureName) {
    textureName = getEmptyTextureMapping();
  }

  side->addChildWithValue("material", textureName);

  const vm::vec3 xAxis = face.textureXAxis();
  addChildFormatted(
    side, "uaxis", "[{} {} {} {}] {}", xAxis.x(), xAxis.y(), xAxis.z(), face.attributes().xOffset(),
    face.attributes().xScale());

  const vm::vec3 yAxis = face.textureYAxis();
  addChildFormatted(
    side, "vaxis", "[{} {} {} {}] {}", yAxis.x(), yAxis.y(), yAxis.z(), face.attributes().yOffset(),
    face.attributes().yScale());

  side->addChildWithValue("rotation", std::to_string(face.attributes().rotation()));

  // Not modifiable yet, so we just use reasonable defaults:
  side->addChildWithValue("lightmapscale", "16");
  side->addChildWithValue("smoothing_groups", "0");

  ValveKeyValuesWriter(stream).write(side, 2);
  return ValveKeyValuesWriter::countOutputLines(side);
}

void VmfFileSerializer::writeBrushesMovedToWorldspawn() {
  for (const auto& it : m_nodeToPrecomputedString) {
    if (it.second.moveToWorldspawn) {
      pushStartLine();
      writePrecomputedString(it.first, it.second);
    }
  }
}

void VmfFileSerializer::writePrecomputedString(
  const Model::Node* brush, const PrecomputedString& pStr) {
  m_stream << pStr.string;
  m_line += pStr.lineCount;
  setFilePosition(brush);
}
} // namespace IO
} // namespace TrenchBroom
