/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "renderer_opengl.h"

namespace SoftGL {

void RendererOpenGL::Create(int w, int h, float near, float far) {
  Renderer::Create(w, h, near, far);
  // TODO
}

void RendererOpenGL::Clear(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RendererOpenGL::DrawMeshTextured(ModelMesh &mesh, glm::mat4 &transform) {

}

void RendererOpenGL::DrawMeshWireframe(ModelMesh &mesh, glm::mat4 &transform) {

}

void RendererOpenGL::DrawLines(ModelLines &lines, glm::mat4 &transform) {

}

void RendererOpenGL::DrawPoints(ModelPoints &points, glm::mat4 &transform) {

}

}
