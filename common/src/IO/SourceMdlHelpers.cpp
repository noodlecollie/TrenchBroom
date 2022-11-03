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

#include "IO/SourceMdlHelpers.h"
#include "Ensure.h"
#include "IO/SourceMdlLayout.h"
#include "Macros.h"
#include "vecmath/forward.h"
#include "vecmath/mat.h"
#include "vecmath/mat_ext.h"
#include "vecmath/vec.h"

namespace TrenchBroom {
namespace IO {
static inline vm::mat4x4f boneToParentMatrix(const SourceMdlLayout::Bone& bone) {
  if (bone.parent < 0) {
    return matrixFromRotAndPos(
             mdlArrayToQuat(bone.quat), vm::vec3f(bone.pos[0], bone.pos[1], bone.pos[2])) *
           STUDIOMDL_ROOT_AXIS_TRANSFORM;
  } else {
    return matrixFromRotAndPos(
      mdlArrayToQuat(bone.quat), vm::vec3f(bone.pos[0], bone.pos[1], bone.pos[2]));
  }
}

vm::quatf mdlArrayToQuat(const SourceMdlLayout::Quaternion& quat) {
  return vm::quatf(quat[3], vm::vec3f(quat[0], quat[1], quat[2]));
}

vm::quatf convertQuaternion64(const SourceMdlLayout::Quaternion64& inQuat) {
  // shift to -1048576, + 1048575, then round down slightly to -1.0 < x < 1.0
  float qX = static_cast<float>(static_cast<int>(inQuat.x) - 1048576) * (1.0f / 1048576.5f);
  float qY = static_cast<float>(static_cast<int>(inQuat.y) - 1048576) * (1.0f / 1048576.5f);
  float qZ = static_cast<float>(static_cast<int>(inQuat.z) - 1048576) * (1.0f / 1048576.5f);
  float qW = sqrtf(1.0f - (qX * qX) - (qY * qY) - (qZ * qZ));

  if (inQuat.wneg) {
    qW = -qW;
  }

  return vm::quatf(qW, vm::vec3f(qX, qY, qZ));
}

vm::quatf convertQuaternion48(const SourceMdlLayout::Quaternion48& inQuat) {
  float qX = static_cast<float>(static_cast<int>(inQuat.x) - 32768) * (1.0f / 32768.0f);
  float qY = static_cast<float>(static_cast<int>(inQuat.y) - 32768) * (1.0f / 32768.0f);
  float qZ = static_cast<float>(static_cast<int>(inQuat.z) - 16384) * (1.0f / 16384.0f);
  float qW = sqrtf(1.0f - (qX * qX) - (qY * qY) - (qZ * qZ));

  if (inQuat.wneg) {
    qW = -qW;
  }

  return vm::quatf(qW, vm::vec3f(qX, qY, qZ));
}

float convertFloat16(const SourceMdlLayout::Float16& inFloat) {
  static constexpr float MAXFLOAT16BITS = 65504.0f;
  static constexpr int FLOAT32BIAS = 127;
  static constexpr int FLOAT16BIAS = 15;

  union Float32 {
    float rawFloat;
    struct {
      uint32_t mantissa : 23;
      uint32_t biased_exponent : 8;
      uint32_t sign : 1;
    } bits;
  };

  if (inFloat.v.bits.biased_exponent == 31) {
    if (inFloat.v.bits.mantissa == 0) {
      // Infinity
      return MAXFLOAT16BITS * ((inFloat.v.bits.sign == 1) ? -1.0f : 1.0f);
    } else {
      // NaN
      return 0.0f;
    }
  }

  Float32 out{};

  if (inFloat.v.bits.biased_exponent == 0 && inFloat.v.bits.mantissa != 0) {
    // denorm
    const float half_denorm = (1.0f / 16384.0f); // 2^-14
    float mantissa = static_cast<float>(inFloat.v.bits.mantissa) / 1024.0f;
    float sgn = (inFloat.v.bits.sign) ? -1.0f : 1.0f;
    out.rawFloat = sgn * mantissa * half_denorm;
  } else {
    // regular number
    uint32_t mantissa = inFloat.v.bits.mantissa;
    uint32_t biased_exponent = inFloat.v.bits.biased_exponent;
    uint32_t sign = static_cast<uint32_t>(inFloat.v.bits.sign) << 31;
    biased_exponent = ((biased_exponent - FLOAT16BIAS + FLOAT32BIAS) * (biased_exponent != 0))
                      << 23;
    mantissa <<= (23 - 10);

    (*reinterpret_cast<uint32_t*>(&out)) = (mantissa | biased_exponent | sign);
  }

  return out.rawFloat;
}

vm::vec3f convertVector48(const SourceMdlLayout::Vector48& inVec) {
  return vm::vec3f(convertFloat16(inVec.x), convertFloat16(inVec.y), convertFloat16(inVec.z));
}

vm::quatf eulerAnglesToQuaternion(const vm::vec3f& angles) {
  float sinYaw = sinf(angles.z() * 0.5f);
  float cosYaw = cosf(angles.z() * 0.5f);
  float sinPitch = sinf(angles.y() * 0.5f);
  float cosPitch = cosf(angles.y() * 0.5f);
  float sinRoll = sinf(angles.x() * 0.5f);
  float cosRoll = cosf(angles.x() * 0.5f);

  float srXcp = sinRoll * cosPitch;
  float crXsp = cosRoll * sinPitch;
  float outX = (srXcp * cosYaw) - (crXsp * sinYaw); // X
  float outY = (crXsp * cosYaw) + (srXcp * sinYaw); // Y

  float crXcp = cosRoll * cosPitch;
  float srXsp = sinRoll * sinPitch;
  float outZ = (crXcp * sinYaw) - (srXsp * cosYaw); // Z
  float outW = (crXcp * cosYaw) + (srXsp * sinYaw); // W (real component)

  return vm::quatf(outW, vm::vec3f(outX, outY, outZ));
}

vm::quatf alignQuaternion(const vm::quatf& in, const vm::quatf& align) {
  // decide if one of the quaternions is backwards
  float a = 0;
  float b = 0;

  for (size_t index = 0; index < 3; ++index) {
    a += (in.v[index] - align.v[index]) * (in.v[index] - align.v[index]);
    b += (in.v[index] + align.v[index]) * (in.v[index] + align.v[index]);
  }

  a += (in.r - align.r) * (in.r - align.r);
  b += (in.r + align.r) * (in.r + align.r);

  if (a > b) {
    return vm::quatf(-in.r, -in.v);
  } else {
    return in;
  }
}

void quaternionToAxisAndAngle(const vm::quatf& in, vm::vec3f& axis, float& angle) {
  angle = vm::to_degrees(2.0f * acosf(in.r));

  if (angle > 180.0f) {
    angle -= 360.0f;
  }

  axis = vm::normalize(in.v);
}

vm::vec3f matrixToEulerAngles(const vm::mat4x4f& mat) {
  float forward[3] = {0, 0, 0};
  float left[3] = {0, 0, 0};
  float up[3] = {0, 0, 0};

  //
  // Extract the basis vectors from the matrix. Since we only need the Z
  // component of the up vector, we don't get X and Y.
  //
  forward[0] = mat[0][0];
  forward[1] = mat[1][0];
  forward[2] = mat[2][0];
  left[0] = mat[0][1];
  left[1] = mat[1][1];
  left[2] = mat[2][1];
  up[2] = mat[2][2];

  float xyDist = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);

  vm::vec3f angles;

  // enough here to get angles?
  if (xyDist > 0.001f) {
    // (yaw)	y = ATAN( forward.y, forward.x );		-- in our space, forward is the X axis
    angles[1] = vm::to_degrees(atan2f(forward[1], forward[0]));

    // (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
    angles[0] = vm::to_degrees(atan2f(-forward[2], xyDist));

    // (roll)	z = ATAN( left.z, up.z );
    angles[2] = vm::to_degrees(atan2f(left[2], up[2]));
  } else // forward is mostly Z, gimbal lock-
  {
    // (yaw)	y = ATAN( -left.x, left.y );			-- forward is mostly z, so use right for yaw
    angles[1] = vm::to_degrees(atan2f(-left[0], left[1]));

    // (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
    angles[0] = vm::to_degrees(atan2f(-forward[2], xyDist));

    // Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
    angles[2] = 0;
  }

  return angles;
}

vm::mat4x4f translationMatrix(const vm::vec3f& translation) {
  // Note that data in this constructor is row-major, even though TB matrices are column-major...
  return vm::mat4x4f(
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, translation[0], translation[1], translation[2], 1});
}

vm::mat4x4f matrixFromRotAndPos(const vm::quatf& rot, const vm::vec3f& pos) {
  return vm::rotation_matrix(rot) * translationMatrix(pos);
}

vm::mat4x4f mat3x4To4x4(const SourceMdlLayout::Mat3x4& mat) {
  // Source matrices are the transpose of TB matrices.
  // This constructor takes arguments in row-major order.
  return vm::mat4x4f(
    mat[0], mat[4], mat[8], 0, mat[1], mat[5], mat[9], 0, mat[2], mat[6], mat[10], 0, mat[3],
    mat[7], mat[11], 1);
}

std::vector<vm::mat4x4f> computeBoneToWorldMatrices(
  const std::vector<SourceMdlLayout::Bone>& bones) {
  std::vector<vm::mat4x4f> boneToWorldMatrices;
  boneToWorldMatrices.resize(bones.size());

  for (size_t index = 0; index < bones.size(); ++index) {
    boneToWorldMatrices[index] = boneToParentMatrix(bones[index]);
  }

  concatenateBoneChainMatrices(bones, boneToWorldMatrices);

  return boneToWorldMatrices;
}

void invertMatrices(std::vector<vm::mat4x4f>& matrices) {
  for (vm::mat4x4f& mat : matrices) {
    auto [invertible, inverse] = vm::invert(mat);
    ensure(invertible, "Expected matrix to be invertible");
    unused(invertible);

    mat = inverse;
  }
}

void concatenateBoneChainMatrices(
  const std::vector<SourceMdlLayout::Bone>& bones, std::vector<vm::mat4x4f>& matrices) {
  ensure(bones.size() == matrices.size(), "Bone and matrix lists have different sizes");

  for (size_t index = 0; index < matrices.size(); ++index) {
    const SourceMdlLayout::Bone& bone = bones[index];
    const int32_t parent = bone.parent;

    if (parent >= 0 && static_cast<size_t>(parent) < matrices.size()) {
      matrices[index] = matrices[index] * matrices[static_cast<size_t>(parent)];
    }
  }
}

// A quick summary of what this actually does:
// The body index is the index of one particular permutation of all the submodels available for all
// the body parts in the overall model. Each time the body index is incremented, it increments the
// index of the submodel we use for the first body part. Once we increment past the total number of
// submodels for the first body part, we roll back to index 0 for that body part and increment the
// submodel index for the *second* body part (think of it like an odometer in a car). This way, we
// can enumerate all possible permutations of submodels across all the body parts.
// The "base" attribute for a body part is basically the sum of all the permutations for all the
// previous body parts. We use it as a divisor for the overall index, so that as later body parts
// are encountered, the body index needs to cross a larger and larger threshold to "increment" to
// the next submodel for that body part.
// Finally, performing a modulo over the number of submodels for the body part provides the "roll
// back to index zero" part of the calculation.
size_t calculateSubmodelIndex(const SourceMdlLayout::BodyPart& bodyPart, size_t bodyIndex) {
  return static_cast<size_t>(
    (bodyIndex / static_cast<size_t>(bodyPart.base)) % static_cast<size_t>(bodyPart.numsubmodels));
}

size_t calculateNextAnimationOffset(
  const SourceMdlLayout::Animation& animation, size_t baseOffset) {
  if (animation.nextoffset != 0) {
    return static_cast<size_t>(
      static_cast<int32_t>(baseOffset) + static_cast<int32_t>(animation.nextoffset));
  } else {
    return 0;
  }
}
} // namespace IO
} // namespace TrenchBroom
