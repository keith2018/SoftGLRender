/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "render/framebuffer.h"
#include "render/opengl/texture_opengl.h"

namespace SoftGL {

class FrameBufferOpenGL : public FrameBuffer {
 public:
  FrameBufferOpenGL() {
    glGenFramebuffers(1, &fbo_);
  }

  ~FrameBufferOpenGL() {
    glDeleteFramebuffers(1, &fbo_);
  }

  inline GLuint GetId() const {
    return fbo_;
  }

  bool IsValid() override {
    if (!fbo_) {
      return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }

  void Bind() override {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  }

  void Unbind() override {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void SetColorAttachment(std::shared_ptr<Texture2D> &color) override {
    color_attachment = color;
    auto *tex = dynamic_cast<Texture2DOpenGL *>(color.get());
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex->GetId(), 0);
  }

  void SetDepthAttachment(std::shared_ptr<TextureDepth> &depth) override {
    depth_attachment = depth;
    auto *buf = dynamic_cast<TextureDepthOpenGL *>(depth.get());
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buf->GetId());
  }

 private:
  GLuint fbo_ = 0;
  std::shared_ptr<Texture2D> color_attachment;
  std::shared_ptr<TextureDepth> depth_attachment;
};

}