/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/model.h"
#include "render/soft/renderer_soft.h"

namespace SoftGL {
namespace View {

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

 private:
  int width_;
  int height_;
  ModelMesh quad_mesh_;
  std::shared_ptr<RendererSoft> renderer_;
};

}
}