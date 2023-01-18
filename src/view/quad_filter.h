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
  QuadFilter(std::shared_ptr<Texture2D> &tex_in,
             std::shared_ptr<Texture2D> &tex_out,
             const std::string &vsSource,
             const std::string &fsSource);

  void Draw();

 private:
  int width_ = 0;
  int height_ = 0;
  bool init_ready_ = false;

  ModelMesh quad_mesh_;
  std::shared_ptr<Renderer> renderer_;
  std::shared_ptr<FrameBuffer> fbo_;
};

}
}
