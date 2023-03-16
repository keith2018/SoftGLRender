/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Viewer.h"
#include "Render/OpenGL/OpenGLUtils.h"
#include "Render/Software/RendererSoft.h"
#include "Render/Software/TextureSoft.h"
#include "Shader/Software/ShaderSoft.h"

namespace SoftGL {
namespace View {

#define CASE_CREATE_SHADER_SOFT(shading, source) case shading: \
  return programSoft->SetShaders(std::make_shared<source::VS>(), std::make_shared<source::FS>())

class ViewerSoftware : public Viewer {
 public:
  ViewerSoftware(Config &config, Camera &camera) : Viewer(config, camera) {}

  void configRenderer() override {
    if (renderer_->isReverseZ() != config_.reverseZ) {
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

  void destroy() override {
    if (renderer_) {
      renderer_->destroy();
    }
  }

  std::shared_ptr<Renderer> createRenderer() override {
    auto renderer = std::make_shared<RendererSoft>();
    if (!renderer->create()) {
      return nullptr;
    }
    return renderer;
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
