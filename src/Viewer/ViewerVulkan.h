/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Viewer.h"
#include "Render/Vulkan/RendererVulkan.h"

namespace SoftGL {
namespace View {

class ViewerVulkan : public Viewer {
 public:
  ViewerVulkan(Config &config, Camera &camera) : Viewer(config, camera) {
  }

  void configRenderer() override {
  }

  void swapBuffer() override {
    auto *vkRenderer = dynamic_cast<RendererVulkan *>(renderer_.get());
    vkRenderer->readPixels([&](uint8_t *buffer, uint32_t width, uint32_t height) -> void {
      GL_CHECK(glBindTexture(GL_TEXTURE_2D, outTexId_));
      GL_CHECK(glTexImage2D(GL_TEXTURE_2D,
                            0,
                            GL_RGBA,
                            width,
                            height,
                            0,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            buffer));
    });
  }

  void destroy() override {
  }

  std::shared_ptr<Renderer> createRenderer() override {
    return std::make_shared<RendererVulkan>();
  }

  bool loadShaders(ShaderProgram &program, ShadingModel shading) override {
    return false;
  }

 private:
};

}
}
