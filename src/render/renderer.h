/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/common.h"
#include "base/model.h"

namespace SoftGL {

class Renderer {
 public:
  virtual void Create(int width, int height, float near, float far) {
    width_ = width;
    height_ = height;
    near_ = near;
    far_ = far;
  };

  virtual void Clear(float r, float g, float b, float a) {};

  virtual void DrawMeshTextured(ModelMesh &mesh, glm::mat4 &transform) {};
  virtual void DrawMeshWireframe(ModelMesh &mesh, glm::mat4 &transform) {};
  virtual void DrawLines(ModelLines &lines, glm::mat4 &transform) {};
  virtual void DrawPoints(ModelPoints &points, glm::mat4 &transform) {};

 protected:
  int width_ = 0;
  int height_ = 0;
  float near_ = 0;
  float far_ = 0;
};

}
