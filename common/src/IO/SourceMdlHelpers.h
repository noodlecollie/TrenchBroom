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

#include "IO/SourceMdlLayout.h"
#include "vecmath/forward.h"
#include "vecmath/mat.h"
#include <vector>

/*
  Notes on conventions between Source and TrenchBroom
  ===================================================

  Or: my attempt to reorient myself in this new world.

  TL;DR:

    * Source matrices are row-major, TB matrices are column-major.
    * Source matrices pre-multiply a column vector, TB matrices post-multiply a row vector.
    * TB matrix transformations are applied left-to-right.
    * StudioMDL uses a left-handed co-ordinate system and TB uses a right-handed one.

  TB matrices are specified in column-major format (ie. mat.v[0] is the vector for column 0).
  Each column specifies manipulations on a specific axis, beginning from X.
  Therefore, to get the translation applied by a matrix, the vector is
  (mat.v[0][3], mat.v[1][3], mat.v[2][3]).
  This means that the translation vector runs along the bottom row of the matrix, which is the
  transpose of the type of matrix I remember dealing with in university.

  This format means that a vector, represented as a row, is post-multiplied by a matrix:

                | . . . . |
  [ x y z 1 ] X | . . . . | = [ . . . . ]
                | . . . . |
                | . . . . | <- This is the translation row

  This is *different* to how matrices work in the Source engine! Matrices used when dealing with MDL
  files are given the name matrix3x4_t, meaning they have 3 rows and 4 columns. This means that the
  final column must represent the translation (the fourth row is the implied [0 0 0 1] row).

                | . . . tx |                     | . . . tx |
  matrix3x4_t = | . . . ty |    As a 4x4 matrix: | . . . ty |
                | . . . tz |                     | . . . tz |
                                                 | 0 0 0  1 |

  Given that the x translation is selected from this matrix by doing mat[0][3], Source matrices must
  be stored in row-major format, where the row is selected first, and then the column.

  Since Source matrices are the transpose of TB matrices, this means that they must transform
  transposed vectors. Source matrices therefore pre-multiply column vectors like so:

  | . . . . |   | x |   | . |
  | . . . . | X | y | = | . |
  | . . . . |   | z |   | . |
  | . . . . |   | 1 |   | . |

  As these matrices are transposes of one another, but also have their data stored in an opposite
  *-major order, it turns out that addressing elements in [a][b] notation is actually the same. Eg.
  translation component x is found in both matrices at index [0][3]. I'm not sure whether it's more
  confusing to have to swap notation between the different types of matrix, or *not* have to swap
  notation even though the matrices are different... I'll try not to think about it.

  Another notable anomaly is that Source MDLs seem to be rotated 90 degrees around Z (right-handed
  rotation), as a vertex on the positive X axis will appear on the positive Y axis in the MDL
  viewer. This appears to be a transform applied automatically by StudioMDL, as it's the case even
  if you use the reference SMD as an animation.

  Additionally, StudioMDL's co-ordinate space uses X as north, Y as east, and Z as up. TB's
  co-ordinate space uses X as east, Y as north, and Z as up. A transform can be applied to any root
  bones in a model to convert from StudioMDL to TB space.
*/

namespace TrenchBroom {
namespace IO {
// StudioMDL is left-handed and TB is right-handed, so we apply this matrix to the root bone of the
// model to convert between these conventions.
static constexpr vm::mat4x4f STUDIOMDL_ROOT_AXIS_TRANSFORM(
  0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

// Dunno if this exists already in TB?
template <typename T, size_t SIZE> static constexpr size_t ArraySize(const T (&)[SIZE]) {
  return SIZE;
}

vm::quatf mdlArrayToQuat(const SourceMdlLayout::Quaternion& quat);
vm::quatf convertQuaternion64(const SourceMdlLayout::Quaternion64& inQuat);
vm::quatf convertQuaternion48(const SourceMdlLayout::Quaternion48& inQuat);

float convertFloat16(const SourceMdlLayout::Float16& inFloat);
vm::vec3f convertVector48(const SourceMdlLayout::Vector48& inVec);

vm::quatf eulerAnglesToQuaternion(const vm::vec3f& angles);
vm::quatf alignQuaternion(const vm::quatf& in, const vm::quatf& align);

void quaternionToAxisAndAngle(const vm::quatf& in, vm::vec3f& axis, float& angle);
vm::vec3f matrixToEulerAngles(const vm::mat4x4f& mat);

// Am I being an idiot? I can't find any function that will produce a translation matrix from a
// vector...
vm::mat4x4f translationMatrix(const vm::vec3f& translation);
vm::mat4x4f matrixFromRotAndPos(const vm::quatf& rot, const vm::vec3f& pos);
vm::mat4x4f mat3x4To4x4(const SourceMdlLayout::Mat3x4& mat);

std::vector<vm::mat4x4f> computeBoneToWorldMatrices(
  const std::vector<SourceMdlLayout::Bone>& bones);

void invertMatrices(std::vector<vm::mat4x4f>& matrices);

void concatenateBoneChainMatrices(
  const std::vector<SourceMdlLayout::Bone>& bones, std::vector<vm::mat4x4f>& matrices);

// Given a body index (which is an index for some permutation of the submodels present in the
// overall model), we work out what the submodel index is for the given body part.
size_t calculateSubmodelIndex(const SourceMdlLayout::BodyPart& bodyPart, size_t bodyIndex);

size_t calculateNextAnimationOffset(const SourceMdlLayout::Animation& animation, size_t baseOffset);
} // namespace IO
} // namespace TrenchBroom
