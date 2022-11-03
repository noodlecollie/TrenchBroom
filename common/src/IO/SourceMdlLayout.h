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

#include <cstdint>

namespace TrenchBroom {
namespace IO {
namespace SourceMdlLayout {
enum AnimationFlag {
  ANIMFLAG_POSITION_IS_VEC48 = 0x01,
  ANIMFLAG_ROTATION_IS_QUAT48 = 0x02,
  ANIMFLAG_POSITION_IS_VALUEPTR = 0x04,
  ANIMFLAG_ROTATION_IS_VALUEPTR = 0x08,
  ANIMFLAG_DELTA = 0x10,
  ANIMFLAG_ROTATION_IS_QUAT64 = 0x20
};

enum BoneFlag {
  BONEFLAG_FIXED_ALIGNMENT = 0x100000
};

#pragma pack(push, 1)

using Vector2D = float[2];
using Vector3D = float[3];
using Quaternion = float[4];
using Mat3x4 = float[12];
using RadianEuler = Vector3D;

// Placeholder data type for a void*.
// It kinf of blows my mind that a pointer is contained in a struct that's
// used for data serialisation, but there we go. Because the Source SDK
// only compiles in 32-bit, pointers are also 32-bit.
using MdlDataPtr = uint32_t;

struct Quaternion48 {
  uint16_t x : 16;
  uint16_t y : 16;
  uint16_t z : 15;
  uint16_t wneg : 1;
};

struct Quaternion64 {
  uint64_t x : 21;
  uint64_t y : 21;
  uint64_t z : 21;
  uint64_t wneg : 1;
};

struct Float16 {
  union Value {
    uint16_t rawWord;
    struct {
      uint16_t mantissa : 10;
      uint16_t biased_exponent : 5;
      uint16_t sign : 1;
    } bits;
  };

  Value v;
};

struct Vector48 {
  Float16 x;
  Float16 y;
  Float16 z;
};

// This struct is mental. I can see back into space-time when I look at this.
struct Header {

  int32_t id;
  int32_t version;
  int32_t checksum; // this has to be the same in the phy and vtx files to load!
  char name[64];
  int32_t length;

  Vector3D eyeposition;   // ideal eye position
  Vector3D illumposition; // illumination center
  Vector3D hull_min;      // ideal movement hull size
  Vector3D hull_max;

  Vector3D view_bbmin; // clipping bounding box
  Vector3D view_bbmax;

  int32_t flags;

  int32_t numbones; // bones
  int32_t boneindex;

  int32_t numbonecontrollers; // bone controllers
  int32_t bonecontrollerindex;

  int32_t numhitboxsets;
  int32_t hitboxsetindex;

  int32_t numlocalanim;   // animations/poses
  int32_t localanimindex; // animation descriptions

  int32_t numlocalseq; // sequences
  int32_t localseqindex;

  int32_t activitylistversion;
  int32_t eventsindexed;

  // raw textures
  int32_t numtextures;
  int32_t textureindex;

  // raw textures search paths
  int32_t numcdtextures;
  int32_t cdtextureindex;

  // replaceable textures tables
  int32_t numskinref;      // number of "slots" for different textures in use across all meshes
  int32_t numskinfamilies; // number of different skins that can be chosen in game
  int32_t skinindex;

  int32_t numbodyparts;
  int32_t bodypartindex;

  // queryable attachable points
  int32_t numlocalattachments;
  int32_t localattachmentindex;

  // animation node to animation node transition graph
  int32_t numlocalnodes;
  int32_t localnodeindex;
  int32_t localnodenameindex;

  int32_t numflexdesc;
  int32_t flexdescindex;

  int32_t numflexcontrollers;
  int32_t flexcontrollerindex;

  int32_t numflexrules;
  int32_t flexruleindex;

  int32_t numikchains;
  int32_t ikchainindex;

  int32_t nummouths;
  int32_t mouthindex;

  int32_t numlocalposeparameters;
  int32_t localposeparamindex;

  int32_t surfacepropindex;

  // Key values
  int32_t keyvalueindex;
  int32_t keyvaluesize;

  int32_t numlocalikautoplaylocks;
  int32_t localikautoplaylockindex;

  float mass;
  int32_t contents;

  // external animations, models, etc.
  int32_t numincludemodels;
  int32_t includemodelindex;

  MdlDataPtr virtualModel;

  // for demand loaded animation blocks
  int32_t szanimblocknameindex;

  int32_t numanimblocks;
  int32_t animblockindex;
  MdlDataPtr animblockModel;

  int32_t bonetablebynameindex;

  MdlDataPtr pVertexBase;
  MdlDataPtr pIndexBase;

  // if STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT is set,
  // this value is used to calculate directional components of lighting
  // on static props
  uint8_t constdirectionallightdot;

  uint8_t rootLOD;
  uint8_t numAllowedRootLODs;

  uint8_t unused[1];

  int32_t unused4; // zero out if version < 47

  int32_t numflexcontrollerui;
  int32_t flexcontrolleruiindex;

  float flVertAnimFixedPointScale;

  int32_t unused3[1];

  int32_t studiohdr2index;

  int32_t unused2[1];
};

struct AnimationDescription {
  int32_t baseptr;

  int32_t sznameindex;

  float fps;      // frames per second
  uint32_t flags; // looping/non-looping flags

  int32_t numframes;

  // piecewise movement
  int32_t nummovements;
  int32_t movementindex;

  int32_t unused1[6]; // remove as appropriate (and zero if loading older versions)

  int32_t animblock;
  int32_t animindex; // non-zero when anim data isn't in sections

  int32_t numikrules;
  int32_t ikruleindex;          // non-zero when IK data is stored in the mdl
  int32_t animblockikruleindex; // non-zero when IK data is stored in animblock file

  int32_t numlocalhierarchy;
  int32_t localhierarchyindex;

  int32_t sectionindex;
  int32_t sectionframes; // number of frames used in each fast lookup section, zero if not used

  int16_t zeroframespan;  // frames per span
  int16_t zeroframecount; // number of spans
  int32_t zeroframeindex;
  float zeroframestalltime; // saved during read stalls
};

struct AnimationSection {
  int32_t animblock;
  int32_t animindex;
};

struct Animation {
  uint8_t bone;
  uint8_t flags; // weighing options

  int16_t nextoffset;
};

struct AnimationValuePtr {
  int16_t offset[3];
};

struct AnimationValue {
  union ValueUnion {
    struct {
      uint8_t valid;
      uint8_t total;
    } num;

    int16_t value;
  };

  ValueUnion v;
};

struct Bone {
  int32_t sznameindex;
  int32_t parent;            // parent bone
  int32_t bonecontroller[6]; // bone controller index, -1 == none

  // default values
  Vector3D pos;
  Quaternion quat;
  RadianEuler rot;

  // compression scale
  Vector3D posscale;
  Vector3D rotscale;

  Mat3x4 poseToBone;
  Quaternion qAlignment;

  int32_t flags;
  int32_t proctype;
  int32_t procindex;      // procedural rule
  int32_t physicsbone;    // index into physically simulated bone
  int32_t surfacepropidx; // index into string tablefor property name
  int32_t contents;       // See BSPFlags.h for the contents flags

  int32_t unused[8]; // remove as appropriate
};

struct BodyPart {
  int32_t sznameindex;
  int32_t numsubmodels;
  int32_t base;
  int32_t submodelindex; // index into submodels array
};

struct Submodel {
  char name[64];

  int32_t type;

  float boundingradius;

  int32_t nummeshes;
  int32_t meshindex;

  // cache purposes
  int32_t numvertices;   // number of unique vertices/normals/texcoords
  int32_t vertexindex;   // vertex Vector
  int32_t tangentsindex; // tangents Vector

  int32_t numattachments;
  int32_t attachmentindex;

  int32_t numeyeballs;
  int32_t eyeballindex;

  int32_t unused[8]; // remove as appropriate
};

struct MeshVertexData {
  MdlDataPtr submodelvertexdata;

  // used for fixup calcs when culling top level lods
  // expected number of mesh verts at desired lod
  int32_t numLODVertexes[8];
};

struct Mesh {
  int32_t material;

  int32_t submodelindex;

  int32_t numvertices;  // number of unique vertices/normals/texcoords
  int32_t vertexoffset; // vertex mstudiovertex_t

  int32_t numflexes; // vertex animation
  int32_t flexindex;

  // special codes for material operations
  int32_t materialtype;
  int32_t materialparam;

  // a unique ordinal for this mesh
  int32_t meshid;

  Vector3D center;

  MeshVertexData vertexdata;

  int32_t unused[8]; // remove as appropriate
};

struct Texture {
  int32_t sznameindex;
  uint32_t flags;
  int32_t used;
  int32_t unused1;
  MdlDataPtr material;
  MdlDataPtr clientmaterial;
  int32_t unused[10];
};

struct SkinRef {
  int16_t index;
};

#pragma pack(pop)
} // namespace SourceMdlLayout
} // namespace IO
} // namespace TrenchBroom
