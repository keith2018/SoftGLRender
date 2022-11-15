/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "viewer.h"
#include "render/soft/renderer_soft.h"
#include "quad_filter.h"

namespace SoftGL {
namespace View {

class ViewerSoft : public Viewer {
 public:
  bool Create(void *window, int width, int height, int outTexId) override;
  void DrawFrame() override;

 private:
  void DrawFrameInternal();
  bool CheckMeshFrustumCull(ModelMesh &mesh, glm::mat4 &transform);
  void DrawModelNodes(ModelNode &node, glm::mat4 &transform, AlphaMode mode, bool wireframe);
  void DrawSkybox(ModelMesh &mesh, glm::mat4 &transform);
  void DrawWorldAxis(ModelLines &lines, glm::mat4 &transform);
  void DrawLights(ModelPoints &points, glm::mat4 &transform);
  void FXAAPostProcess();

  void InitShaderBase(ShaderContext &shader_context, glm::mat4 &model_matrix);
  void InitShaderTextured(ShaderContext &shader_context, ModelMesh &mesh, glm::mat4 &model_matrix);
  void InitShaderSkybox(ShaderContext &shader_context, glm::mat4 &model_matrix);
  void InitShaderFXAA(ShaderContext &shader_context);

  void BindShaderUniforms(ShaderContext &shader_context, glm::mat4 &model_matrix, float alpha_cutoff = 0.5f);
  void BindShaderTextures(ModelMesh &mesh, std::shared_ptr<BaseShaderUniforms> &uniforms);
  static Texture *GetMeshTexture(ModelMesh &mesh, TextureType type);

 private:
  std::shared_ptr<RendererSoft> renderer_;

  Buffer<glm::u8vec4> *out_color_ = nullptr;
  AAType aa_type_ = AAType_NONE;

  // FXAA
  std::shared_ptr<QuadFilter> fxaa_filter_;
  Texture fxaa_tex_;
};

}
}