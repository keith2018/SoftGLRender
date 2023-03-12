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
  Viewer(Config &config, Camera &camera) : config_(config), cameraMain_(camera) {}

  virtual void create(int width, int height, int outTexId);

  virtual void configRenderer();

  virtual void drawFrame(DemoScene &scene);

  virtual void swapBuffer() = 0;

  virtual void destroy();

 protected:
  virtual std::shared_ptr<Renderer> createRenderer() = 0;
  virtual bool loadShaders(ShaderProgram &program, ShadingModel shading) = 0;

 private:
  void processFXAASetup();
  void processFXAADraw();

  void drawScene(bool floor, bool skybox);

  void drawPoints(ModelPoints &points, glm::mat4 &transform);
  void drawLines(ModelLines &lines, glm::mat4 &transform);
  void drawMeshBaseColor(ModelMesh &mesh, bool wireframe);
  void drawMeshTextured(ModelMesh &mesh, float specular);
  void drawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe);
  void drawSkybox(ModelSkybox &skybox, glm::mat4 &transform);

  void pipelineSetup(ModelVertexes &vertexes, Material &material,
                     const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniformBlocks,
                     const std::function<void(RenderState &rs)> &extraStates = nullptr);
  void pipelineDraw(ModelVertexes &vertexes, Material &material);

  void setupMainBuffers();
  void setupShowMapBuffers();
  void setupMainColorBuffer(bool multiSample);
  void setupMainDepthBuffer(bool multiSample);

  void setupVertexArray(ModelVertexes &vertexes);
  void setupRenderStates(RenderState &rs, bool blend) const;
  bool setupShaderProgram(Material &material);
  void setupTextures(Material &material);
  void setupSamplerUniforms(Material &material);
  void setupMaterial(Material &material, const std::unordered_map<int, std::shared_ptr<UniformBlock>> &uniformBlocks);

  void updateUniformScene();
  void updateUniformModel(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &proj);
  void updateUniformMaterial(const glm::vec4 &color, float specular = 1.f);

  bool initSkyboxIBL(ModelSkybox &skybox);
  bool iBLEnabled();
  void updateIBLTextures(Material &material);
  void updateShadowTextures(Material &material);

  static std::set<std::string> generateShaderDefines(Material &material);
  static size_t getShaderProgramCacheKey(ShadingModel shading, const std::set<std::string> &defines);

  std::shared_ptr<Texture> createTextureCubeDefault(int width, int height, bool mipmaps = false);
  std::shared_ptr<Texture> createTexture2DDefault(int width, int height, TextureFormat format, bool mipmaps = false);
  bool checkMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform);

 protected:
  Config &config_;

  Camera &cameraMain_;
  std::shared_ptr<Camera> cameraDepth_ = nullptr;
  Camera *camera_ = nullptr;

  DemoScene *scene_ = nullptr;

  int width_ = 0;
  int height_ = 0;
  int outTexId_ = 0;

  UniformsScene uniformsScene_{};
  UniformsModel uniformsModel_{};
  UniformsMaterial uniformsMaterial_{};

  std::shared_ptr<Renderer> renderer_ = nullptr;

  // main fbo
  std::shared_ptr<FrameBuffer> fboMain_ = nullptr;
  std::shared_ptr<Texture> texColorMain_ = nullptr;
  std::shared_ptr<Texture> texDepthMain_ = nullptr;

  // shadow map
  std::shared_ptr<FrameBuffer> fboShadow_ = nullptr;
  std::shared_ptr<Texture> texDepthShadow_ = nullptr;
  std::shared_ptr<Texture> shadowPlaceholder_ = nullptr;

  // fxaa
  std::shared_ptr<QuadFilter> fxaaFilter_ = nullptr;
  std::shared_ptr<Texture> texColorFxaa_ = nullptr;

  // ibl
  std::shared_ptr<Texture> iblPlaceholder_ = nullptr;

  std::shared_ptr<UniformBlock> uniformBlockScene_;
  std::shared_ptr<UniformBlock> uniformsBlockModel_;
  std::shared_ptr<UniformBlock> uniformsBlockMaterial_;

  // program cache
  std::unordered_map<size_t, std::shared_ptr<ShaderProgram>> programCache_;
};

}
}
