/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/glm_inc.h"

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
  BlendFunction blend_func_rgb = BlendFunc_ADD;
  BlendFactor blend_src_rgb = BlendFactor_ONE;
  BlendFactor blend_dst_rgb = BlendFactor_ZERO;

  BlendFunction blend_func_alpha = BlendFunc_ADD;
  BlendFactor blend_src_alpha = BlendFactor_ONE;
  BlendFactor blend_dst_alpha = BlendFactor_ZERO;

  void SetBlendFactor(BlendFactor src, BlendFactor dst) {
    blend_src_rgb = src;
    blend_src_alpha = src;
    blend_dst_rgb = dst;
    blend_dst_alpha = dst;
  }

  void SetBlendFunc(BlendFunction func) {
    blend_func_rgb = func;
    blend_func_alpha = func;
  }
};

struct RenderState {
  bool blend = false;
  BlendParameters blend_parameters;

  bool depth_test = false;
  bool depth_mask = true;
  DepthFunction depth_func = DepthFunc_LESS;

  bool cull_face = false;
  PolygonMode polygon_mode = PolygonMode_FILL;

  float line_width = 1.f;
  float point_size = 1.f;
};

struct ClearState {
  bool depth_flag = true;
  bool color_flag = true;
  glm::vec4 clear_color = glm::vec4(0.f);
};

}
