/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "render/texture.h"
#include "render/opengl/enums_opengl.h"
#include "render/opengl/opengl_utils.h"

namespace SoftGL {

class Texture2DOpenGL : public Texture2D {
 public:
  explicit Texture2DOpenGL(bool multi_sample = false) {
    Texture::multi_sample = multi_sample;
    target_ = multi_sample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    GL_CHECK(glGenTextures(1, &texId_));
  }

  ~Texture2DOpenGL() {
    GL_CHECK(glDeleteTextures(1, &texId_));
  }

  int GetId() const override {
    return (int) texId_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    auto &sampler_2d = dynamic_cast<Sampler2DDesc &>(sampler);
    GL_CHECK(glBindTexture(target_, texId_));
    if (!multi_sample) {
      GL_CHECK(glTexParameteri(target_, GL_TEXTURE_WRAP_S, OpenGL::ConvertWrap(sampler_2d.wrap_s)));
      GL_CHECK(glTexParameteri(target_, GL_TEXTURE_WRAP_T, OpenGL::ConvertWrap(sampler_2d.wrap_t)));
      GL_CHECK(glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, OpenGL::ConvertFilter(sampler_2d.filter_min)));
      GL_CHECK(glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, OpenGL::ConvertFilter(sampler_2d.filter_mag)));
    }

    use_mipmaps = sampler_2d.use_mipmaps;
  }

  void SetImageData(const std::vector<std::shared_ptr<BufferRGBA>> &buffers) override {
    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();
    GL_CHECK(glBindTexture(target_, texId_));
    if (multi_sample) {
      GL_CHECK(glTexImage2DMultisample(target_, 4, GL_RGBA, width, height, GL_TRUE));
    } else {
      GL_CHECK(glTexImage2D(target_,
                            0,
                            GL_RGBA,
                            width,
                            height,
                            0,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            buffers[0]->GetRawDataPtr()));
    }
    if (!multi_sample && use_mipmaps) {
      GL_CHECK(glGenerateMipmap(target_));
    }
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;
    GL_CHECK(glBindTexture(target_, texId_));
    if (multi_sample) {
      GL_CHECK(glTexImage2DMultisample(target_, 4, GL_RGBA, w, h, GL_TRUE));
    } else {
      GL_CHECK(glTexImage2D(target_, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
    }
    if (!multi_sample && use_mipmaps) {
      GL_CHECK(glGenerateMipmap(target_));
    }
  }

 private:
  GLuint texId_ = 0;
  GLenum target_ = 0;
  bool use_mipmaps = false;
};

class TextureCubeOpenGL : public TextureCube {
 public:
  TextureCubeOpenGL() {
    GL_CHECK(glGenTextures(1, &texId_));
  }

  ~TextureCubeOpenGL() {
    GL_CHECK(glDeleteTextures(1, &texId_));
  }

  int GetId() const override {
    return (int) texId_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    auto &sampler_cube = dynamic_cast<SamplerCubeDesc &>(sampler);
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, texId_));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, OpenGL::ConvertWrap(sampler_cube.wrap_s)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, OpenGL::ConvertWrap(sampler_cube.wrap_t)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, OpenGL::ConvertWrap(sampler_cube.wrap_r)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP,
                             GL_TEXTURE_MIN_FILTER,
                             OpenGL::ConvertFilter(sampler_cube.filter_min)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP,
                             GL_TEXTURE_MAG_FILTER,
                             OpenGL::ConvertFilter(sampler_cube.filter_mag)));

    use_mipmaps = sampler_cube.use_mipmaps;
  }

  void SetImageData(const std::vector<std::shared_ptr<BufferRGBA>> &buffers) override {
    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, texId_));

    for (int i = 0; i < 6; i++) {
      GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                            0,
                            GL_RGBA,
                            width,
                            height,
                            0,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            buffers[i]->GetRawDataPtr()));
    }
    if (use_mipmaps) {
      GL_CHECK(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
    }
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, texId_));
    for (int i = 0; i < 6; i++) {
      GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                            0,
                            GL_RGBA,
                            w,
                            h,
                            0,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            nullptr));
    }
    if (use_mipmaps) {
      GL_CHECK(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
    }
  }

 private:
  GLuint texId_ = 0;
  bool use_mipmaps = false;
};

class TextureDepthOpenGL : public TextureDepth {
 public:
  explicit TextureDepthOpenGL(bool multi_sample = false) {
    Texture::multi_sample = multi_sample;
    GL_CHECK(glGenRenderbuffers(1, &rbId_));
  }

  ~TextureDepthOpenGL() {
    GL_CHECK(glDeleteRenderbuffers(1, &rbId_));
  }

  int GetId() const override {
    return (int) rbId_;
  }

  void InitImageData(int w, int h) override {
    if (width == w && height == h) {
      return;
    }
    width = w;
    height = h;
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, rbId_));
    if (multi_sample) {
      GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, width, height));
    } else {
      GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height));
    }
  }

 private:
  GLuint rbId_ = 0;
};

}
