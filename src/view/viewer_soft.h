/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "viewer.h"
#include "render/opengl/opengl_utils.h"
#include "render/soft/renderer_soft.h"
#include "render/soft/texture_soft.h"
#include "shader/soft/shader_soft.h"

namespace SoftGL {
namespace View {

#define CASE_CREATE_SHADER_SOFT(shading, source) case shading: \
  return program_soft->SetShaders(std::make_shared<source::VS>(), std::make_shared<source::FS>())

class ViewerSoft : public Viewer {
 public:
  ViewerSoft(Config &config, Camera &camera) : Viewer(config, camera) {}

  void ConfigRenderer() override {
    if (renderer_->GetReverseZ() != config_.reverse_z) {
      tex_depth_shadow_ = nullptr;
    }
    renderer_->SetReverseZ(config_.reverse_z);
    renderer_->SetEarlyZ(config_.early_z);
  }

  void SwapBuffer() override {
    auto *tex_out = dynamic_cast<Texture2DSoft<RGBA> *>(tex_color_main_.get());
    auto buffer = tex_out->GetImage().GetBuffer()->buffer;
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, outTexId_));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D,
                          0,
                          GL_RGBA,
                          (int) buffer->GetWidth(),
                          (int) buffer->GetHeight(),
                          0,
                          GL_RGBA,
                          GL_UNSIGNED_BYTE,
                          buffer->GetRawDataPtr()));
  }

  std::shared_ptr<Renderer> CreateRenderer() override {
    return std::make_shared<RendererSoft>();
  }

  bool LoadShaders(ShaderProgram &program, ShadingModel shading) override {
    auto *program_soft = dynamic_cast<ShaderProgramSoft *>(&program);
    switch (shading) {
      CASE_CREATE_SHADER_SOFT(Shading_BaseColor, ShaderBasic);
      CASE_CREATE_SHADER_SOFT(Shading_BlinnPhong, ShaderBlinnPhong);
      CASE_CREATE_SHADER_SOFT(Shading_PBR, ShaderPbrIBL);
      CASE_CREATE_SHADER_SOFT(Shading_Skybox, ShaderSkybox);
      CASE_CREATE_SHADER_SOFT(Shading_FXAA, ShaderFXAA);
      CASE_CREATE_SHADER_SOFT(Shading_IBL_Irradiance, ShaderIBLIrradiance);
      CASE_CREATE_SHADER_SOFT(Shading_IBL_Prefilter, ShaderIBLPrefilter);
      default:
        break;
    }

    return false;
  }
};

}
}
