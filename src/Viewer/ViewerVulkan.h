/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Viewer.h"
#include "Render/Vulkan/RendererVulkan.h"
#include "Render/Vulkan/TextureVulkan.h"

namespace SoftGL {
namespace View {

#define TO_STR(S) #S
#define SHADER_PATH(source, suffix) shaders/GLSL/source.suffix
#define SHADER_PATH_STR(S) TO_STR(S)

#define CASE_CREATE_SHADER_VK(shading, source) case shading: \
  return programVK->compileAndLinkGLSLFile(SHADER_PATH_STR(SHADER_PATH(source, vert)), \
                                           SHADER_PATH_STR(SHADER_PATH(source, frag)))

class ViewerVulkan : public Viewer {
 public:
  ViewerVulkan(Config &config, Camera &camera) : Viewer(config, camera) {
  }

  void configRenderer() override {
    camera_->setReverseZ(config_.reverseZ);
    cameraDepth_->setReverseZ(config_.reverseZ);
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
      CASE_CREATE_SHADER_VK(Shading_BaseColor, BasicGLSL);
      CASE_CREATE_SHADER_VK(Shading_BlinnPhong, BlinnPhongGLSL);
      CASE_CREATE_SHADER_VK(Shading_PBR, PbrGLSL);
      CASE_CREATE_SHADER_VK(Shading_Skybox, SkyboxGLSL);
      CASE_CREATE_SHADER_VK(Shading_IBL_Irradiance, IBLIrradianceGLSL);
      CASE_CREATE_SHADER_VK(Shading_IBL_Prefilter, IBLPrefilterGLSL);
      CASE_CREATE_SHADER_VK(Shading_FXAA, FxaaGLSL);
      default:
        break;
    }

    return false;
  }

 private:
};

}
}
