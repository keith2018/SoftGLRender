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
  Viewer(Config &config, Camera &camera) : config_(config), camera_main_(camera) {}

  virtual void Create(int width, int height, int outTexId);

  virtual void ConfigRenderer();

  virtual void DrawFrame(DemoScene &scene);

  virtual void SwapBuffer() = 0;

  virtual void Destroy();

 protected:
  virtual std::shared_ptr<Renderer> CreateRenderer() = 0;
  virtual bool LoadShaders(ShaderProgram &program, ShadingModel shading) = 0;

 private:
  void FXAASetup();
  void FXAADraw();

  void DrawScene(bool skybox);

  void DrawPoints(ModelPoints &points, glm::mat4 &transform);
  void DrawLines(ModelLines &lines, glm::mat4 &transform);
  void DrawMeshBaseColor(ModelMesh &mesh, bool wireframe);
  void DrawMeshTextured(ModelMesh &mesh, float specular);
  void DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe);
  void DrawSkybox(ModelSkybox &skybox, glm::mat4 &transform);

  void PipelineSetup(ModelVertexes &vertexes,
                     Material &material,
                     const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniform_blocks,
                     const std::function<void(RenderState &rs)> &extra_states = nullptr);
  void PipelineDraw(ModelVertexes &vertexes, Material &material);

  void SetupMainBuffers();
  void SetupShowMapBuffers();
  void SetupMainColorBuffer(bool multi_sample);
  void SetupMainDepthBuffer(bool multi_sample);

  void SetupVertexArray(ModelVertexes &vertexes);
  void SetupRenderStates(RenderState &rs, bool blend) const;
  bool SetupShaderProgram(Material &material);
  void SetupTextures(Material &material);
  void SetupSamplerUniforms(Material &material);
  void SetupMaterial(Material &material, const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniform_blocks);

  void UpdateUniformScene();
  void UpdateUniformModel(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &proj);
  void UpdateUniformMaterial(const glm::vec4 &color, float specular = 1.f);

  bool InitSkyboxIBL(ModelSkybox &skybox);
  bool IBLEnabled();
  void UpdateIBLTextures(Material &material);
  void UpdateShadowTextures(Material &material);

  static std::set<std::string> GenerateShaderDefines(Material &material);
  static size_t GetShaderProgramCacheKey(ShadingModel shading, const std::set<std::string> &defines);

  std::shared_ptr<Texture> CreateTextureCubeDefault(int width, int height, bool mipmaps = false);
  std::shared_ptr<Texture> CreateTexture2DDefault(int width, int height, TextureFormat format, bool mipmaps = false);
  bool CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform);

 protected:
  Config &config_;

  Camera &camera_main_;
  std::shared_ptr<Camera> camera_depth_ = nullptr;
  Camera *camera_ = nullptr;

  DemoScene *scene_ = nullptr;

  int width_ = 0;
  int height_ = 0;
  int outTexId_ = 0;

  UniformsScene uniforms_scene_{};
  UniformsModel uniforms_model_{};
  UniformsMaterial uniforms_material_{};

  std::shared_ptr<Renderer> renderer_ = nullptr;

  // main fbo
  std::shared_ptr<FrameBuffer> fbo_main_ = nullptr;
  std::shared_ptr<Texture> tex_color_main_ = nullptr;
  std::shared_ptr<Texture> tex_depth_main_ = nullptr;

  // shadow map
  std::shared_ptr<FrameBuffer> fbo_shadow_ = nullptr;
  std::shared_ptr<Texture> tex_depth_shadow_ = nullptr;
  std::shared_ptr<Texture> shadow_placeholder_ = nullptr;

  // fxaa
  std::shared_ptr<QuadFilter> fxaa_filter_ = nullptr;
  std::shared_ptr<Texture> tex_color_fxaa_ = nullptr;

  // ibl
  std::shared_ptr<Texture> ibl_placeholder_ = nullptr;

  std::shared_ptr<UniformBlock> uniform_block_scene_;
  std::shared_ptr<UniformBlock> uniforms_block_model_;
  std::shared_ptr<UniformBlock> uniforms_block_material_;

  std::unordered_map<size_t, std::shared_ptr<ShaderProgram>> program_cache_;
};

}
}
