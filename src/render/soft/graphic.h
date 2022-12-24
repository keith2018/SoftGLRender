/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/common.h"
#include "base/frame_buffer.h"

namespace SoftGL {

class Graphic {
 public:
  bool Barycentric(glm::aligned_vec4 *vert,
                   glm::aligned_vec4 &v0,
                   glm::aligned_vec4 &p,
                   glm::aligned_vec4 &bc);

  void DrawLine(glm::vec4 v0, glm::vec4 v1, const glm::u8vec4 &color);

  void DrawPoint(glm::vec2 pos, float size, float depth, const glm::u8vec4 &color);

  std::function<void(int x, int y, float depth, const glm::u8vec4 &color)> DrawImpl;
};

}