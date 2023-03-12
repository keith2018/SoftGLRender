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
    int width = texColorMain_->width;
    int height = texColorMain_->height;
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
