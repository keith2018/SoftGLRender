/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "viewer.h"
#include "render/opengl/renderer_opengl.h"
#include "glad/glad.h"

namespace SoftGL {
namespace View {

class ViewerOpenGL : public Viewer {
 public:
  bool Create(void *window, int width, int height, int outTexId) override;
  void DrawFrame() override;

 private:
  void DrawFrameInternal();

 private:
  std::shared_ptr<RendererOpenGL> renderer_ = nullptr;
  GLuint fbo_ = 0;
};

}
}