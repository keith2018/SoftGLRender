/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Render/RenderState.h"

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

static inline GLint convertCubeFace(CubeMapFace face) {
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

static inline GLint convertDepthFunc(DepthFunction func) {
  switch (func) {
    CASE_CVT_GL(DepthFunc_, NEVER);
    CASE_CVT_GL(DepthFunc_, LESS);
    CASE_CVT_GL(DepthFunc_, EQUAL);
    CASE_CVT_GL(DepthFunc_, LEQUAL);
    CASE_CVT_GL(DepthFunc_, GREATER);
    CASE_CVT_GL(DepthFunc_, NOTEQUAL);
    CASE_CVT_GL(DepthFunc_, GEQUAL);
    CASE_CVT_GL(DepthFunc_, ALWAYS);
    default:
      break;
  }
  return 0;
}

static inline GLint convertBlendFactor(BlendFactor factor) {
  switch (factor) {
    CASE_CVT_GL(BlendFactor_, ZERO);
    CASE_CVT_GL(BlendFactor_, ONE);
    CASE_CVT_GL(BlendFactor_, SRC_COLOR);
    CASE_CVT_GL(BlendFactor_, SRC_ALPHA);
    CASE_CVT_GL(BlendFactor_, DST_COLOR);
    CASE_CVT_GL(BlendFactor_, DST_ALPHA);
    CASE_CVT_GL(BlendFactor_, ONE_MINUS_SRC_COLOR);
    CASE_CVT_GL(BlendFactor_, ONE_MINUS_SRC_ALPHA);
    CASE_CVT_GL(BlendFactor_, ONE_MINUS_DST_COLOR);
    CASE_CVT_GL(BlendFactor_, ONE_MINUS_DST_ALPHA);
    default:
      break;
  }
  return 0;
}

static inline GLint convertBlendFunction(BlendFunction func) {
  switch (func) {
    case BlendFunc_ADD:               return GL_FUNC_ADD;
    case BlendFunc_SUBTRACT:          return GL_FUNC_SUBTRACT;
    case BlendFunc_REVERSE_SUBTRACT:  return GL_FUNC_REVERSE_SUBTRACT;
    case BlendFunc_MIN:               return GL_MIN;
    case BlendFunc_MAX:               return GL_MAX;
    default:
      break;
  }
  return 0;
}

static inline GLint convertPolygonMode(PolygonMode mode) {
  switch (mode) {
    CASE_CVT_GL(PolygonMode_, POINT);
    CASE_CVT_GL(PolygonMode_, LINE);
    CASE_CVT_GL(PolygonMode_, FILL);
    default:
      break;
  }
  return 0;
}

static inline GLint convertDrawMode(PrimitiveType type) {
  switch (type) {
    case Primitive_POINT:       return GL_POINTS;
    case Primitive_LINE:        return GL_LINES;
    case Primitive_TRIANGLE:    return GL_TRIANGLES;
    default:
      break;
  }
  return 0;
}

}
}
