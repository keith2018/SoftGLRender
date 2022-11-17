/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "../renderer.h"
#include "glad/glad.h"


namespace SoftGL {

class RendererOpenGL : public Renderer {
 public:
  void Create(int width, int height, float near, float far) override;
  void Clear(float r, float g, float b, float a) override;

  void DrawMeshTextured(ModelMesh &mesh, glm::mat4 &transform) override;
  void DrawMeshWireframe(ModelMesh &mesh, glm::mat4 &transform) override;
  void DrawLines(ModelLines &lines, glm::mat4 &transform) override;
  void DrawPoints(ModelPoints &points, glm::mat4 &transform) override;

 private:

};

}
