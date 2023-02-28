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
             const std::function<bool(ShaderProgram &program)> &shader_func);

  void SetTextures(std::shared_ptr<Texture2D> &tex_in,
                   std::shared_ptr<Texture2D> &tex_out);

  void Draw();

 private:
  int width_ = 0;
  int height_ = 0;
  bool init_ready_ = false;

  ModelMesh quad_mesh_;
  std::shared_ptr<Renderer> renderer_;
  std::shared_ptr<FrameBuffer> fbo_;

  UniformsQuadFilter uniform_filter_{};
  std::shared_ptr<UniformBlock> uniform_block_filter_;
  std::shared_ptr<UniformSampler> uniform_tex_in_;
};

}
}
