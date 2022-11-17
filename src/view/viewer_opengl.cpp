/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#include "viewer_opengl.h"

namespace SoftGL {
namespace View {

bool ViewerOpenGL::Create(void *window, int width, int height, int outTexId) {
  bool success = Viewer::Create(window, width, height, outTexId);
  if (!success) {
    return false;
  }

  renderer_ = std::make_shared<RendererOpenGL>();

  GLint lastFbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFbo);

  // create fbo
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

  // depth buffer
  GLuint depthRenderBuffer;
  glGenRenderbuffers(1, &depthRenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);

  // color buffer
  glBindTexture(GL_TEXTURE_2D, outTexId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outTexId, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    return false;
  }

  glViewport(0, 0, width_, height_);
  glBindFramebuffer(GL_FRAMEBUFFER, lastFbo);
  return true;
}

void ViewerOpenGL::DrawFrame() {
  Viewer::DrawFrame();

  GLint lastFbo = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFbo);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  DrawFrameInternal();

  glBindFramebuffer(GL_FRAMEBUFFER, lastFbo);
}

void ViewerOpenGL::DrawFrameInternal() {
  auto clear_color = settings_->clear_color;
  renderer_->Clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
}

}
}