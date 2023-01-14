/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/render_state.h"


namespace SoftGL {
namespace OpenGL {

#define CVT_CASE(PRE, TOKEN) case PRE##TOKEN: return GL_##TOKEN

static inline GLint ConvertWrap(WrapMode mode) {
  switch (mode) {
    CVT_CASE(Wrap_, REPEAT);
    CVT_CASE(Wrap_, MIRRORED_REPEAT);
    CVT_CASE(Wrap_, CLAMP_TO_EDGE);
    CVT_CASE(Wrap_, CLAMP_TO_BORDER);
    default:break;
  }
  return GL_REPEAT;
}

static inline GLint ConvertFilter(FilterMode mode) {
  switch (mode) {
    CVT_CASE(Filter_, LINEAR);
    CVT_CASE(Filter_, NEAREST);
    CVT_CASE(Filter_, LINEAR_MIPMAP_LINEAR);
    CVT_CASE(Filter_, LINEAR_MIPMAP_NEAREST);
    CVT_CASE(Filter_, NEAREST_MIPMAP_LINEAR);
    CVT_CASE(Filter_, NEAREST_MIPMAP_NEAREST);
    default:break;
  }
  return GL_NEAREST;
}

static inline GLint ConvertCubeFace(CubeMapFace face) {
  switch (face) {
    CVT_CASE(, TEXTURE_CUBE_MAP_POSITIVE_X);
    CVT_CASE(, TEXTURE_CUBE_MAP_NEGATIVE_X);
    CVT_CASE(, TEXTURE_CUBE_MAP_POSITIVE_Y);
    CVT_CASE(, TEXTURE_CUBE_MAP_NEGATIVE_Y);
    CVT_CASE(, TEXTURE_CUBE_MAP_POSITIVE_Z);
    CVT_CASE(, TEXTURE_CUBE_MAP_NEGATIVE_Z);
    default:break;
  }
  return 0;
}

static inline GLint ConvertDepthFunc(DepthFunc func) {
  switch (func) {
    CVT_CASE(Depth_, NEVER);
    CVT_CASE(Depth_, LESS);
    CVT_CASE(Depth_, EQUAL);
    CVT_CASE(Depth_, LEQUAL);
    CVT_CASE(Depth_, GREATER);
    CVT_CASE(Depth_, NOTEQUAL);
    CVT_CASE(Depth_, GEQUAL);
    CVT_CASE(Depth_, ALWAYS);
    default:break;
  }
  return 0;
}

static inline GLint ConvertBlendFactor(BlendFactor factor) {
  switch (factor) {
    CVT_CASE(Factor_, ZERO);
    CVT_CASE(Factor_, ONE);
    CVT_CASE(Factor_, SRC_COLOR);
    CVT_CASE(Factor_, SRC_ALPHA);
    CVT_CASE(Factor_, DST_COLOR);
    CVT_CASE(Factor_, DST_ALPHA);
    CVT_CASE(Factor_, ONE_MINUS_SRC_COLOR);
    CVT_CASE(Factor_, ONE_MINUS_SRC_ALPHA);
    CVT_CASE(Factor_, ONE_MINUS_DST_COLOR);
    CVT_CASE(Factor_, ONE_MINUS_DST_ALPHA);
    default:break;
  }
  return 0;
}

static inline GLint ConvertPolygonMode(PolygonMode mode) {
  switch (mode) {
    CVT_CASE(, LINE);
    CVT_CASE(, FILL);
    default:break;
  }
  return 0;
}

static inline GLint ConvertPrimitiveType(PrimitiveType type) {
  switch (type) {
    CVT_CASE(Primitive_, POINTS);
    CVT_CASE(Primitive_, LINES);
    CVT_CASE(Primitive_, TRIANGLES);
    default:break;
  }
  return 0;
}

}
}