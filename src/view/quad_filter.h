/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "render/renderer.h"
#include "model.h"

namespace SoftGL {
namespace View {

class QuadFilter {
 public:
  QuadFilter(const std::shared_ptr<Renderer> &renderer,
             const std::function<bool(ShaderProgram &program)> &shaderFunc);

  void setTextures(std::shared_ptr<Texture> &texIn, std::shared_ptr<Texture> &texOut);

  void draw();

 private:
  int width_ = 0;
  int height_ = 0;
  bool initReady_ = false;

  ModelMesh quadMesh_;
  std::shared_ptr<Renderer> renderer_;
  std::shared_ptr<FrameBuffer> fbo_;

  UniformsQuadFilter uniformFilter_{};
  std::shared_ptr<UniformBlock> uniformBlockFilter_;
  std::shared_ptr<UniformSampler> uniformTexIn_;
};

}
}
