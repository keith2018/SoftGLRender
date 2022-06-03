/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "renderer/model.h"
#include "renderer/renderer.h"

namespace SoftGL {
namespace App {

class QuadFilter {
 public:
  QuadFilter(int width, int height);

  void Clear(float r, float g, float b, float a);
  void Draw();

  inline std::shared_ptr<Buffer<glm::u8vec4>> &GetFrameColor() {
    return renderer_->GetFrameColor();
  };

  inline ShaderContext &GetShaderContext() {
    return renderer_->GetShaderContext();
  }

  inline const std::shared_ptr<Renderer> &GetRenderer() const {
    return renderer_;
  }

 private:
  int width_;
  int height_;
  ModelMesh quad_mesh_;
  std::shared_ptr<Renderer> renderer_;
};

}
}