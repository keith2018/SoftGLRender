/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <immintrin.h>

#define GLM_FORCE_ALIGNED
#define GLM_FORCE_INLINE
#define GLM_FORCE_AVX2
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_aligned.hpp>


namespace SoftGL {

#define FILE_SEPARATOR '/'

#define SOFTGL_ALIGNMENT 32
#define PTR_ADDR(p) ((size_t)(p))

#ifdef _MSC_VER
#define MM_F32(v, i) v.m128_f32[i]
#else
#define MM_F32(v, i) v[i]
#endif


#define PI            3.14159265359f
#define TWO_PI        6.28318530718f
#define FOUR_PI       12.56637061436f
#define FOUR_PI2      39.47841760436f
#define INV_PI        0.31830988618f
#define INV_TWO_PI    0.15915494309f
#define INV_FOUR_PI   0.07957747155f
#define HALF_PI       1.57079632679f
#define INV_HALF_PI   0.636619772367f

struct Vertex {
  glm::vec3 a_position;
  glm::vec2 a_texCoord;
  glm::vec3 a_normal;

  glm::vec3 a_tangent;
};

enum ShadingType {
  ShadingType_BASE_COLOR = 0,
  ShadingType_BLINN_PHONG,
  ShadingType_PBR_BRDF,
};

enum FrustumClipMask {
  POSITIVE_X = 1 << 0,
  NEGATIVE_X = 1 << 1,
  POSITIVE_Y = 1 << 2,
  NEGATIVE_Y = 1 << 3,
  POSITIVE_Z = 1 << 4,
  NEGATIVE_Z = 1 << 5,
};

const int FrustumClipMaskArray[6] = {
    FrustumClipMask::POSITIVE_X,
    FrustumClipMask::NEGATIVE_X,
    FrustumClipMask::POSITIVE_Y,
    FrustumClipMask::NEGATIVE_Y,
    FrustumClipMask::POSITIVE_Z,
    FrustumClipMask::NEGATIVE_Z,
};

const glm::vec4 FrustumClipPlane[6] = {
    {-1, 0, 0, 1},
    {1, 0, 0, 1},
    {0, -1, 0, 1},
    {0, 1, 0, 1},
    {0, 0, -1, 1},
    {0, 0, 1, 1}
};

struct DepthRange {
  void Set(float near_, float far_) {
    near = near_;
    far = far_;
    diff = far_ - near_;
    sum = far_ + near_;
  }

  float near = 0;
  float far = 0;
  float diff = 0;   // far - near
  float sum = 0;    // far + near
};

enum DepthFunc {
  Depth_NEVER,
  Depth_LESS,
  Depth_EQUAL,
  Depth_LEQUAL,
  Depth_GREATER,
  Depth_NOTEQUAL,
  Depth_GEQUAL,
  Depth_ALWAYS,
};

inline bool DepthTest(float &a, float &b, DepthFunc func) {
  switch (func) {
    case Depth_NEVER:return false;
    case Depth_LESS:return a < b;
    case Depth_EQUAL:return std::fabs(a - b) <= std::numeric_limits<float>::epsilon();
    case Depth_LEQUAL:return a <= b;
    case Depth_GREATER:return a > b;
    case Depth_NOTEQUAL:return std::fabs(a - b) > std::numeric_limits<float>::epsilon();
    case Depth_GEQUAL:return a >= b;
    case Depth_ALWAYS:return true;
  }
  return a < b;
}

enum AlphaMode {
  Alpha_Opaque,
  Alpha_Mask,
  Alpha_Blend,
};

enum AAType {
  AAType_NONE,
  AAType_SSAA,
  AAType_FXAA,
};

}
