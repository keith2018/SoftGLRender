/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Viewer.h"
#include "Render/Vulkan/RendererVulkan.h"
#include "Render/Vulkan/TextureVulkan.h"
#include "Shader/GLSL/ShaderGLSL.h"

namespace SoftGL {
namespace View {

#define CASE_CREATE_SHADER_VK(shading, source) case shading: \
  return programVK->compileAndLinkGLSL(source##_VS, source##_FS)

class ViewerVulkan : public Viewer {
 public:
  ViewerVulkan(Config &config, Camera &camera) : Viewer(config, camera) {
  }

  void configRenderer() override {
    if (renderer_->isReverseZ() != config_.reverseZ) {
      texDepthShadow_ = nullptr;
    }
    renderer_->setReverseZ(config_.reverseZ);

    // not available
    config_.earlyZ = false;
  }

  void swapBuffer() override {
    auto *vkTex = dynamic_cast<TextureVulkan *>(texColorMain_.get());
    vkTex->readPixels(0, 0, [&](uint8_t *buffer, uint32_t width, uint32_t height, uint32_t rowStride) -> void {
      GL_CHECK(glBindTexture(GL_TEXTURE_2D, outTexId_));
      GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, rowStride / sizeof(RGBA)));
      GL_CHECK(glTexImage2D(GL_TEXTURE_2D,
                            0,
                            GL_RGBA,
                            width,
                            height,
                            0,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            buffer));
      GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
    });
  }

  void destroy() override {
    Viewer::destroy();
    if (renderer_) {
      renderer_->destroy();
    }
  }

  std::shared_ptr<Renderer> createRenderer() override {
    auto renderer = std::make_shared<RendererVulkan>();
    if (!renderer->create()) {
      return nullptr;
    }
    return renderer;
  }

  bool loadShaders(ShaderProgram &program, ShadingModel shading) override {
    auto *programVK = dynamic_cast<ShaderProgramVulkan *>(&program);
    switch (shading) {
      CASE_CREATE_SHADER_VK(Shading_BaseColor, BASIC);
      CASE_CREATE_SHADER_VK(Shading_BlinnPhong, BLINN_PHONG);
      CASE_CREATE_SHADER_VK(Shading_PBR, PBR_IBL);
      CASE_CREATE_SHADER_VK(Shading_Skybox, SKYBOX);
      CASE_CREATE_SHADER_VK(Shading_IBL_Irradiance, IBL_IRRADIANCE);
      CASE_CREATE_SHADER_VK(Shading_IBL_Prefilter, IBL_PREFILTER);
      CASE_CREATE_SHADER_VK(Shading_FXAA, FXAA);
      default:
        break;
    }

    return false;
  }

 private:
};

}
}
