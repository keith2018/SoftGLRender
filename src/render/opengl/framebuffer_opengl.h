/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "render/framebuffer.h"
#include "render/opengl/texture_opengl.h"
#include "render/opengl/opengl_utils.h"


namespace SoftGL {

class FrameBufferOpenGL : public FrameBuffer {
 public:
  FrameBufferOpenGL() {
    GL_CHECK(glGenFramebuffers(1, &fbo_));
  }

  ~FrameBufferOpenGL() {
    GL_CHECK(glDeleteFramebuffers(1, &fbo_));
  }

  int GetId() const override {
    return (int) fbo_;
  }

  bool IsValid() override {
    if (!fbo_) {
      return false;
    }

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }

  void Bind() override {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
  }

  void Unbind() override {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  }

  void SetColorAttachment(std::shared_ptr<Texture2D> &color) override {
    color_attachment = color;
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GL_CHECK(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color->GetId(), 0));
  }

  void SetDepthAttachment(std::shared_ptr<TextureDepth> &depth) override {
    depth_attachment = depth;
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth->GetId()));
  }

 private:
  GLuint fbo_ = 0;
  std::shared_ptr<Texture2D> color_attachment;
  std::shared_ptr<TextureDepth> depth_attachment;
};

}
