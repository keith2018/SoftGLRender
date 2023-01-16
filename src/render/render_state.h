/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/glm_inc.h"

namespace SoftGL {

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

enum BlendFactor {
  Factor_ZERO,
  Factor_ONE,
  Factor_SRC_COLOR,
  Factor_SRC_ALPHA,
  Factor_DST_COLOR,
  Factor_DST_ALPHA,
  Factor_ONE_MINUS_SRC_COLOR,
  Factor_ONE_MINUS_SRC_ALPHA,
  Factor_ONE_MINUS_DST_COLOR,
  Factor_ONE_MINUS_DST_ALPHA,
};

enum PolygonMode {
  LINE,
  FILL,
};

struct RenderState {
  bool blend = false;
  BlendFactor blend_src = Factor_ONE;
  BlendFactor blend_dst = Factor_ZERO;

  bool depth_test = false;
  bool depth_mask = true;
  DepthFunc depth_func = Depth_LESS;

  bool cull_face = false;
  PolygonMode polygon_mode = FILL;

  float line_width = 1.f;
  float point_size = 1.f;
};

struct ClearState {
  bool depth_flag = true;
  bool color_flag = true;
  glm::vec4 clear_color = glm::vec4(0.f);
};

}
