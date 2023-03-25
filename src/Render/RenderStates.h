/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/GLMInc.h"

namespace SoftGL {

enum DepthFunction {
  DepthFunc_NEVER,
  DepthFunc_LESS,
  DepthFunc_EQUAL,
  DepthFunc_LEQUAL,
  DepthFunc_GREATER,
  DepthFunc_NOTEQUAL,
  DepthFunc_GEQUAL,
  DepthFunc_ALWAYS,
};

enum BlendFactor {
  BlendFactor_ZERO,
  BlendFactor_ONE,
  BlendFactor_SRC_COLOR,
  BlendFactor_SRC_ALPHA,
  BlendFactor_DST_COLOR,
  BlendFactor_DST_ALPHA,
  BlendFactor_ONE_MINUS_SRC_COLOR,
  BlendFactor_ONE_MINUS_SRC_ALPHA,
  BlendFactor_ONE_MINUS_DST_COLOR,
  BlendFactor_ONE_MINUS_DST_ALPHA,
};

enum BlendFunction {
  BlendFunc_ADD,
  BlendFunc_SUBTRACT,
  BlendFunc_REVERSE_SUBTRACT,
  BlendFunc_MIN,
  BlendFunc_MAX,
};

enum PolygonMode {
  PolygonMode_POINT,
  PolygonMode_LINE,
  PolygonMode_FILL,
};

struct BlendParameters {
  BlendFunction blendFuncRgb = BlendFunc_ADD;
  BlendFactor blendSrcRgb = BlendFactor_ONE;
  BlendFactor blendDstRgb = BlendFactor_ZERO;

  BlendFunction blendFuncAlpha = BlendFunc_ADD;
  BlendFactor blendSrcAlpha = BlendFactor_ONE;
  BlendFactor blendDstAlpha = BlendFactor_ZERO;

  void SetBlendFactor(BlendFactor src, BlendFactor dst) {
    blendSrcRgb = src;
    blendSrcAlpha = src;
    blendDstRgb = dst;
    blendDstAlpha = dst;
  }

  void SetBlendFunc(BlendFunction func) {
    blendFuncRgb = func;
    blendFuncAlpha = func;
  }
};

enum PrimitiveType {
  Primitive_POINT,
  Primitive_LINE,
  Primitive_TRIANGLE,
};

struct RenderStates {
  bool blend = false;
  BlendParameters blendParams;

  bool depthTest = false;
  bool depthMask = true;
  DepthFunction depthFunc = DepthFunc_LESS;

  bool cullFace = false;
  PrimitiveType primitiveType = Primitive_TRIANGLE;
  PolygonMode polygonMode = PolygonMode_FILL;

  float lineWidth = 1.f;
  float pointSize = 1.f;
};

struct ClearStates {
  bool depthFlag = false;
  bool colorFlag = false;
  glm::vec4 clearColor = glm::vec4(0.f);
};

}
