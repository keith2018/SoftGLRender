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
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      LOGE("glCheckFramebufferStatus: %x", status);
      return false;
    }

    return true;
  }

  void SetColorAttachment(std::shared_ptr<Texture> &color, int level) override {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_COLOR_ATTACHMENT0,
                                    color->multi_sample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
                                    color->GetId(),
                                    level));
    FrameBuffer::SetColorAttachment(color, level);
  }

  void SetColorAttachment(std::shared_ptr<Texture> &color, CubeMapFace face, int level) override {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_COLOR_ATTACHMENT0,
                                    OpenGL::ConvertCubeFace(face),
                                    color->GetId(),
                                    level));
    FrameBuffer::SetColorAttachment(color, face, level);
  }

  void SetDepthAttachment(std::shared_ptr<Texture> &depth) override {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_DEPTH_ATTACHMENT,
                                    depth->multi_sample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
                                    depth->GetId(),
                                    0));
    FrameBuffer::SetDepthAttachment(depth);
  }

  void Bind() const {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
  }

 private:
  GLuint fbo_ = 0;
};

}
