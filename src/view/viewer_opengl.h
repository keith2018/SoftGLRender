/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "viewer.h"
#include "render/opengl/renderer_opengl.h"
#include "glad/glad.h"

namespace SoftGL {
namespace View {

class ViewerOpenGL : public Viewer {
 public:
  ~ViewerOpenGL() { Destroy(); };
  bool Create(void *window, int width, int height, int outTexId) override;
  void DrawFrame() override;
  void Destroy();

 private:
  void DrawFrameInternal();
  bool CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform);
  void DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe);
  void DrawSkybox(ModelMesh &mesh, glm::mat4 &transform);
  void DrawWorldAxis(ModelLines &lines, glm::mat4 &transform);
  void DrawLights(ModelPoints &points, glm::mat4 &transform);

  void UpdateUniformMVP(UniformsMVP &uniforms, glm::mat4 &transform);
  void UpdateUniformLight(UniformsLight &uniforms);

 private:
  std::shared_ptr<RendererOpenGL> renderer_ = nullptr;
  GLuint fbo_ = 0;
};

}
}