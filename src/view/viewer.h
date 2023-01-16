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

class Viewer {
 public:
  Viewer(Config &config, Camera &camera) : config_(config), camera_(camera) {}

  virtual void Create(int width, int height, int outTexId);

  virtual void DrawFrame(DemoScene &scene);

  virtual void Destroy();

 private:
  void DrawPoints(ModelPoints &points, glm::mat4 &transform);
  void DrawLines(ModelLines &lines, glm::mat4 &transform);
  void DrawSkybox(ModelSkybox &skybox, glm::mat4 &transform);
  void DrawMeshWireframe(ModelMesh &mesh);
  void DrawMeshTextured(ModelMesh &mesh);
  void DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe);
  void PipelineDraw(VertexArray &vertexes,
                    Material &material,
                    const std::vector<std::shared_ptr<Uniform>> &uniform_blocks,
                    bool blend,
                    const std::function<void(RenderState &rs)> &extra_states);

  void SetupVertexArray(VertexArray &vertexes);
  void SetupRenderStates(RenderState &rs, bool blend, const std::function<void(RenderState &rs)> &extra) const;
  void SetupShaderProgram(Material &material, const std::set<std::string> &shader_defines = {});
  void SetupTextures(Material &material);
  void SetupSamplerUniforms(Material &material,
                            std::vector<std::shared_ptr<Uniform>> &sampler_uniforms,
                            std::set<std::string> &shader_defines);
  void SetupMaterial(Material &material, const std::vector<std::shared_ptr<Uniform>> &uniform_blocks);
  void StartRenderPipeline(VertexArray &vertexes, Material &material);

  void UpdateUniformScene();
  void UpdateUniformMVP(const glm::mat4 &transform, bool skybox = false);
  void UpdateUniformColor(const glm::vec4 &color);

  void InitSkyboxIBL(ModelSkybox &skybox);

  static size_t GetShaderProgramCacheKey(ShadingModel shading, const std::set<std::string> &defines);
  static glm::mat4 AdjustModelCenter(BoundingBox &bounds);

  std::shared_ptr<TextureCube> CreateTextureCubeDefault(int width, int height);
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

  UniformsScene uniforms_scene_{};
  UniformsMVP uniforms_mvp_{};
  UniformsColor uniforms_color_{};

  std::shared_ptr<UniformBlock> uniform_block_scene_;
  std::shared_ptr<UniformBlock> uniforms_block_mvp_;
  std::shared_ptr<UniformBlock> uniforms_block_color_;

  std::unordered_map<size_t, std::shared_ptr<ShaderProgram>> program_cache_;
};

}
}
