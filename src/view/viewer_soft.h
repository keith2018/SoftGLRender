/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "viewer.h"
#include "render/soft/renderer_soft.h"
#include "render/soft/texture_soft.h"

namespace SoftGL {
namespace View {

class ViewerSoft : public Viewer {
 public:
  ViewerSoft(Config &config, Camera &camera) : Viewer(config, camera) {}

  void SwapBuffer() override {
    auto *tex_out = dynamic_cast<Texture2DSoft *>(color_tex_out_.get());
    auto buffer = tex_out->GetBuffer();
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 (int) buffer->GetWidth(),
                 (int) buffer->GetHeight(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 buffer->GetRawDataPtr());
  }

  std::shared_ptr<Renderer> CreateRenderer() override {
    return std::make_shared<RendererSoft>();
  }

  bool LoadShaders(ShaderProgram &program, ShadingModel shading) override {
    return false;
  }
};

}
}
