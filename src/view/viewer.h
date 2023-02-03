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
#include "quad_filter.h"

namespace SoftGL {
namespace View {

class Viewer {
 public:
  Viewer(Config &config, Camera &camera) : config_(config), camera_(camera) {}

  virtual void Create(int width, int height, int outTexId);

  virtual void DrawFrame(DemoScene &scene);

  virtual void SwapBuffer() = 0;

  virtual void Destroy();

 protected:
  virtual std::shared_ptr<Renderer> CreateRenderer() = 0;
  virtual bool LoadShaders(ShaderProgram &program, ShadingModel shading) = 0;

 private:
  void FXAASetup();
  void FXAADraw();

  void DrawPoints(ModelPoints &points, glm::mat4 &transform);
  void DrawLines(ModelLines &lines, glm::mat4 &transform);
  void DrawMeshWireframe(ModelMesh &mesh);
  void DrawMeshTextured(ModelMesh &mesh);
  void DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe);
  void DrawSkybox(ModelSkybox &skybox, glm::mat4 &transform);

  void PipelineSetup(VertexArray &vertexes,
                     Material &material,
                     const std::vector<std::shared_ptr<UniformBlock>> &uniform_blocks,
                     bool blend,
                     const std::function<void(RenderState &rs)> &extra_states);
  void PipelineDraw(VertexArray &vertexes, Material &material);

  void SetupVertexArray(VertexArray &vertexes);
  void SetupRenderStates(RenderState &rs, bool blend, const std::function<void(RenderState &rs)> &extra) const;
  bool SetupShaderProgram(Material &material, const std::set<std::string> &shader_defines = {});
  void SetupTextures(Material &material, std::set<std::string> &shader_defines);
  void SetupSamplerUniforms(Material &material);
  void SetupMaterial(Material &material, const std::vector<std::shared_ptr<UniformBlock>> &uniform_blocks);

  void UpdateUniformScene();
  void UpdateUniformMVP(const glm::mat4 &transform, bool skybox = false);
  void UpdateUniformColor(const glm::vec4 &color);

  void InitSkyboxIBL(ModelSkybox &skybox);
  bool IBLEnabled();
  void UpdateIBLTextures(Material &material);

  static size_t GetShaderProgramCacheKey(ShadingModel shading, const std::set<std::string> &defines);
  static glm::mat4 AdjustModelCenter(BoundingBox &bounds);

  std::shared_ptr<TextureCube> CreateTextureCubeDefault(int width, int height, bool mipmaps = false);
  bool CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform);

 protected:
  Config &config_;
  Camera &camera_;
  DemoScene *scene_ = nullptr;

  int width_ = 0;
  int height_ = 0;
  int outTexId_ = 0;

  UniformsScene uniforms_scene_{};
  UniformsMVP uniforms_mvp_{};
  UniformsColor uniforms_color_{};

  std::shared_ptr<Renderer> renderer_ = nullptr;
  std::shared_ptr<QuadFilter> fxaa_filter_ = nullptr;

  std::shared_ptr<FrameBuffer> fbo_ = nullptr;
  std::shared_ptr<Texture2D> color_tex_out_ = nullptr;
  std::shared_ptr<Texture2D> color_tex_fxaa_ = nullptr;
  std::shared_ptr<TextureCube> ibl_placeholder_ = nullptr;

  std::shared_ptr<UniformBlock> uniform_block_scene_;
  std::shared_ptr<UniformBlock> uniforms_block_mvp_;
  std::shared_ptr<UniformBlock> uniforms_block_color_;

  std::unordered_map<size_t, std::shared_ptr<ShaderProgram>> program_cache_;
};

}
}
