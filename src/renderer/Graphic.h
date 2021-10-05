/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Common.h"
#include "FrameBuffer.h"

namespace SoftGL {

class Graphic {
 public:
  bool Barycentric(glm::aligned_vec4 *vert,
                   glm::aligned_vec4 &v0,
                   glm::aligned_vec4 &p,
                   glm::aligned_vec4 &bc);

  void DrawLine(glm::vec4 v0, glm::vec4 v1, const glm::vec4 &color);

  void DrawCircle(int xc, int yc, int r, float depth, const glm::vec4 &color, bool fill = true);

  std::function<void(int x, int y, float depth, const glm::vec4 &color)> DrawPoint;

 private:
  void DrawCirclePoint(int xc, int yc, int x, int y, float depth, const glm::vec4 &color);
};

}