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
  return programSoft->SetShaders(std::make_shared<source::VS>(), std::make_shared<source::FS>())

class ViewerSoft : public Viewer {
 public:
  ViewerSoft(Config &config, Camera &camera) : Viewer(config, camera) {}

  void configRenderer() override {
    if (renderer_->getReverseZ() != config_.reverseZ) {
      texDepthShadow_ = nullptr;
    }
    renderer_->setReverseZ(config_.reverseZ);
    renderer_->setEarlyZ(config_.earlyZ);
  }

  void swapBuffer() override {
    auto *texOut = dynamic_cast<Texture2DSoft<RGBA> *>(texColorMain_.get());
    auto buffer = texOut->getImage().getBuffer()->buffer;
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, outTexId_));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D,
                          0,
                          GL_RGBA,
                          (int) buffer->getWidth(),
                          (int) buffer->getHeight(),
                          0,
                          GL_RGBA,
                          GL_UNSIGNED_BYTE,
                          buffer->getRawDataPtr()));
  }

  std::shared_ptr<Renderer> createRenderer() override {
    return std::make_shared<RendererSoft>();
  }

  bool loadShaders(ShaderProgram &program, ShadingModel shading) override {
    auto *programSoft = dynamic_cast<ShaderProgramSoft *>(&program);
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
