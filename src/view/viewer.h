/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/glm_inc.h"
#include "render/renderer.h"
#include "model.h"
#include "config.h"
#include "camera.h"

namespace SoftGL {
namespace View {

struct UniformsScene {
  glm::int32_t u_enablePointLight;
  glm::int32_t u_enableIBL;

  glm::vec3 u_ambientColor;
  glm::vec3 u_cameraPosition;
  glm::vec3 u_pointLightPosition;
  glm::vec3 u_pointLightColor;
};

struct UniformsMVP {
  glm::mat4 u_modelMatrix;
  glm::mat4 u_modelViewProjectionMatrix;
  glm::mat3 u_inverseTransposeModelMatrix;
};

struct UniformsColor {
  glm::vec4 u_baseColor;
};

class Viewer {
 public:
  Viewer(Config &config, Camera &camera) : config_(config), camera_(camera) {}

  virtual void Create(int width, int height, int outTexId);

  virtual void DrawFrame(DemoScene &scene);

  virtual void Destroy();

 private:
  void DrawPoints(ModelPoints &points, glm::mat4 &transform);
  void DrawLines(ModelLines &lines, glm::mat4 &transform);
  void DrawMeshWireframe(ModelMesh &mesh);
  void DrawMeshTextured(ModelMesh &mesh);
  void DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe);

  void SetupVertexArray(VertexArray &vertexes);
  void SetupRenderStates(RenderState &rs, bool blend = false) const;
  void SetupUniforms(Material &material, const std::vector<std::vector<std::shared_ptr<Uniform>>> &uniforms);
  void SetupTextures(TexturedMaterial &material, std::vector<std::shared_ptr<Uniform>> &sampler_uniforms,
                     std::vector<std::string> &shader_defines);
  void SetupShaderProgram(Material &material, ShadingModel shading_model,
                          const std::vector<std::string> &shader_defines = {});

  void StartRenderPipeline(VertexArray &vertexes, Material &material);

  void UpdateUniformScene();
  void UpdateUniformMVP(const glm::mat4 &transform);
  void UpdateUniformColor(const glm::vec4 &color);

  glm::mat4 AdjustModelCenter(BoundingBox &bounds);
  bool CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform);
 protected:
  int width_ = 0;
  int height_ = 0;
  int outTexId_ = 0;

  Config &config_;
  Camera &camera_;

  std::shared_ptr<Renderer> renderer_;
  std::shared_ptr<FrameBuffer> fbo_;

  ClearState clear_state_;

  UniformsScene uniforms_scene_;
  UniformsMVP uniforms_mvp_;
  UniformsColor uniforms_color_;

  std::shared_ptr<UniformBlock> uniform_block_scene_;
  std::shared_ptr<UniformBlock> uniforms_block_mvp_;
  std::shared_ptr<UniformBlock> uniforms_block_color_;

  std::vector<std::shared_ptr<Uniform>> common_uniforms;
};

}
}
