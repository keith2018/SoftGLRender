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

  void Bind() {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
  }

  void SetColorAttachment(std::shared_ptr<Texture2D> &color, int level) override {
    color_tex_type = TextureType_2D;
    color_attachment_2d.tex = color;
    color_attachment_2d.level = level;
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_2D,
                                    color->GetId(),
                                    level));
    color_ready = true;
  }

  void SetColorAttachment(std::shared_ptr<TextureCube> &color, CubeMapFace face, int level) override {
    color_tex_type = TextureType_CUBE;
    color_attachment_cube.tex = color;
    color_attachment_cube.face = face;
    color_attachment_cube.level = level;
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_COLOR_ATTACHMENT0,
                                    OpenGL::ConvertCubeFace(face),
                                    color->GetId(),
                                    level));
    color_ready = true;
  }

  void SetDepthAttachment(std::shared_ptr<TextureDepth> &depth) override {
    depth_attachment = depth;
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));
    GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                       GL_DEPTH_ATTACHMENT,
                                       GL_RENDERBUFFER,
                                       depth->GetId()));
    depth_ready = true;
  }

  std::shared_ptr<Texture> GetColorAttachment() override {
    switch (color_tex_type) {
      case TextureType_2D:
        return color_attachment_2d.tex;
      case TextureType_CUBE:
        return color_attachment_cube.tex;
      default:
        break;
    }
    return nullptr;
  }

  std::shared_ptr<Texture> GetDepthAttachment() override {
    return depth_attachment;
  }

  void UpdateAttachmentsSize(int width, int height) override {
    // color attachment
    if (color_ready) {
      switch (color_tex_type) {
        case TextureType_2D:
          if (color_attachment_2d.tex->width != width || color_attachment_2d.tex->height != height) {
            color_attachment_2d.tex->InitImageData(width, height);
            SetColorAttachment(color_attachment_2d.tex, color_attachment_2d.level);
            break;
          }
          break;
        case TextureType_CUBE:
          if (color_attachment_cube.tex->width != width || color_attachment_cube.tex->height != height) {
            color_attachment_cube.tex->InitImageData(width, height);
            SetColorAttachment(color_attachment_cube.tex, color_attachment_cube.face, color_attachment_cube.level);
            break;
          }
          break;
        default:
          break;
      }
    }

    // depth attachment
    if (depth_ready) {
      if (depth_attachment->width != width || depth_attachment->height != height) {
        depth_attachment->InitImageData(width, height);
        SetDepthAttachment(depth_attachment);
      }
    }
  }

 private:
  GLuint fbo_ = 0;
  bool color_ready = false;
  bool depth_ready = false;

  TextureType color_tex_type = TextureType_2D;
  struct {
    std::shared_ptr<Texture2D> tex;
    int level = 0;
  } color_attachment_2d;

  struct {
    std::shared_ptr<TextureCube> tex;
    CubeMapFace face = TEXTURE_CUBE_MAP_POSITIVE_X;
    int level = 0;
  } color_attachment_cube;

  std::shared_ptr<TextureDepth> depth_attachment;
};

}
