/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Viewer.h"
#include "Config.h"
#include "Render/Vulkan/RendererVulkan.h"
#include "Render/Vulkan/TextureVulkan.h"
#include "Render/Vulkan/VKGLInterop.h"

namespace SoftGL {
namespace View {

#define CASE_CREATE_SHADER_VK(shading, source) case shading: \
  return programVK->compileAndLinkGLSLFile(SHADER_GLSL_DIR + #source + ".vert", \
                                           SHADER_GLSL_DIR + #source + ".frag")

class ViewerVulkan : public Viewer {
 public:
  ViewerVulkan(Config &config, Camera &camera) : Viewer(config, camera) {
  }

  void configRenderer() override {
    camera_->setReverseZ(config_.reverseZ);
    cameraDepth_->setReverseZ(config_.reverseZ);

    if (glInterop_ && VKGLInterop::isAvailable()) {
      glInterop_->signalGLComplete();
    }
  }

  int swapBuffer() override {
    auto *vkTex = dynamic_cast<TextureVulkan *>(texColorMain_.get());

    // first round or texture changed
    if (glInterop_ != &vkTex->getGLInterop()) {
      glInterop_ = &vkTex->getGLInterop();
      if (VKGLInterop::isAvailable()) {
        // create new GL texture
        if (interopOutTex_ > 0) {
          GL_CHECK(glDeleteTextures(1, &interopOutTex_));
        }
        interopOutTex_ = createGLTexture2D();

        // set texture storage
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, interopOutTex_));
        glInterop_->setGLTexture2DStorage(interopOutTex_, 1, GL_RGBA8, vkTex->width, vkTex->height);
      }
    }

    if (glInterop_ && VKGLInterop::isAvailable()) {
      glInterop_->waitGLReady();
      return (int) interopOutTex_;
    }

    vkTex->readPixels(0, 0, [&](uint8_t *buffer, uint32_t width, uint32_t height, uint32_t rowStride) -> void {
      GL_CHECK(glBindTexture(GL_TEXTURE_2D, outTexId_));
      GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, rowStride / sizeof(RGBA)));
      GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D,
                               0,
                               0,
                               0,
                               width,
                               height,
                               GL_RGBA,
                               GL_UNSIGNED_BYTE,
                               buffer));
      GL_CHECK(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
    });
    return outTexId_;
  }

  bool create(int width, int height, int outTexId) override {
    glInterop_ = nullptr;
    return Viewer::create(width, height, outTexId);
  }

  void destroy() override {
    Viewer::destroy();
    if (interopOutTex_ > 0) {
      GL_CHECK(glDeleteTextures(1, &interopOutTex_));
    }
    glInterop_ = nullptr;
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

  static GLuint createGLTexture2D() {
    unsigned int texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
  }

 private:
  GLuint interopOutTex_ = 0;
  VKGLInterop *glInterop_ = nullptr;
};

}
}
