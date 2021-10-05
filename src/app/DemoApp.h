/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "renderer/Renderer.h"
#include "Camera.h"
#include "OrbitController.h"
#include "QuadFilter.h"
#include "Settings.h"
#include "SettingPanel.h"

namespace SoftGL {
namespace App {

class DemoApp {
 public:
  virtual ~DemoApp();

  bool Create(void *window, int width, int height);
  void DrawFrame();

  inline void DrawUI() {
    settingPanel_->OnDraw();
  }

  inline Buffer<glm::vec4> *GetFrame() {
    return out_color_;
  }

  inline void UpdateSize(int width, int height) {
    settingPanel_->UpdateSize(width, height);
  }

  inline void ToggleUIShowState() {
    settingPanel_->ToggleShowState();
  }

  inline bool WantCaptureKeyboard() {
    return settingPanel_->WantCaptureKeyboard();
  }

  inline bool WantCaptureMouse() {
    return settingPanel_->WantCaptureMouse();
  }

  inline SmoothOrbitController *GetOrbitController() {
    return orbitController_.get();
  }

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
  int width_ = 0;
  int height_ = 0;

  Buffer<glm::vec4> *out_color_ = nullptr;
  AAType aa_type_ = AAType_NONE;

  std::shared_ptr<Renderer> renderer_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<SmoothOrbitController> orbitController_;
  std::shared_ptr<Settings> settings_;
  std::shared_ptr<SettingPanel> settingPanel_;
  std::shared_ptr<ModelLoader> model_loader_;

  // FXAA
  std::shared_ptr<QuadFilter> fxaa_filter_;
  Texture fxaa_tex_;
};

}
}