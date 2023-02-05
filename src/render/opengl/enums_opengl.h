/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/render_state.h"

namespace SoftGL {
namespace OpenGL {

#define CASE_CVT_GL(PRE, TOKEN) case PRE##TOKEN: return GL_##TOKEN

static inline GLint ConvertWrap(WrapMode mode) {
  switch (mode) {
    CASE_CVT_GL(Wrap_, REPEAT);
    CASE_CVT_GL(Wrap_, MIRRORED_REPEAT);
    CASE_CVT_GL(Wrap_, CLAMP_TO_EDGE);
    CASE_CVT_GL(Wrap_, CLAMP_TO_BORDER);
    default:
      break;
  }
  return GL_REPEAT;
}

static inline GLint ConvertFilter(FilterMode mode) {
  switch (mode) {
    CASE_CVT_GL(Filter_, LINEAR);
    CASE_CVT_GL(Filter_, NEAREST);
    CASE_CVT_GL(Filter_, LINEAR_MIPMAP_LINEAR);
    CASE_CVT_GL(Filter_, LINEAR_MIPMAP_NEAREST);
    CASE_CVT_GL(Filter_, NEAREST_MIPMAP_LINEAR);
    CASE_CVT_GL(Filter_, NEAREST_MIPMAP_NEAREST);
    default:
      break;
  }
  return GL_NEAREST;
}

static inline GLint ConvertCubeFace(CubeMapFace face) {
  switch (face) {
    CASE_CVT_GL(, TEXTURE_CUBE_MAP_POSITIVE_X);
    CASE_CVT_GL(, TEXTURE_CUBE_MAP_NEGATIVE_X);
    CASE_CVT_GL(, TEXTURE_CUBE_MAP_POSITIVE_Y);
    CASE_CVT_GL(, TEXTURE_CUBE_MAP_NEGATIVE_Y);
    CASE_CVT_GL(, TEXTURE_CUBE_MAP_POSITIVE_Z);
    CASE_CVT_GL(, TEXTURE_CUBE_MAP_NEGATIVE_Z);
    default:
      break;
  }
  return 0;
}

static inline GLint ConvertDepthFunc(DepthFunc func) {
  switch (func) {
    CASE_CVT_GL(Depth_, NEVER);
    CASE_CVT_GL(Depth_, LESS);
    CASE_CVT_GL(Depth_, EQUAL);
    CASE_CVT_GL(Depth_, LEQUAL);
    CASE_CVT_GL(Depth_, GREATER);
    CASE_CVT_GL(Depth_, NOTEQUAL);
    CASE_CVT_GL(Depth_, GEQUAL);
    CASE_CVT_GL(Depth_, ALWAYS);
    default:
      break;
  }
  return 0;
}

static inline GLint ConvertBlendFactor(BlendFactor factor) {
  switch (factor) {
    CASE_CVT_GL(Factor_, ZERO);
    CASE_CVT_GL(Factor_, ONE);
    CASE_CVT_GL(Factor_, SRC_COLOR);
    CASE_CVT_GL(Factor_, SRC_ALPHA);
    CASE_CVT_GL(Factor_, DST_COLOR);
    CASE_CVT_GL(Factor_, DST_ALPHA);
    CASE_CVT_GL(Factor_, ONE_MINUS_SRC_COLOR);
    CASE_CVT_GL(Factor_, ONE_MINUS_SRC_ALPHA);
    CASE_CVT_GL(Factor_, ONE_MINUS_DST_COLOR);
    CASE_CVT_GL(Factor_, ONE_MINUS_DST_ALPHA);
    default:
      break;
  }
  return 0;
}

static inline GLint ConvertPolygonMode(PolygonMode mode) {
  switch (mode) {
    CASE_CVT_GL(Polygon_, POINT);
    CASE_CVT_GL(Polygon_, LINE);
    CASE_CVT_GL(Polygon_, FILL);
    default:
      break;
  }
  return 0;
}

static inline GLint ConvertDrawMode(PrimitiveType type) {
  switch (type) {
    case Primitive_POINT:
      return GL_POINTS;
    case Primitive_LINE:
      return GL_LINES;
    case Primitive_TRIANGLE:
      return GL_TRIANGLES;
    default:
      break;
  }
  return 0;
}

}
}