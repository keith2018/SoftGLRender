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

struct TextureOpenGLDesc {
  GLint internalformat;
  GLenum format;
  GLenum type;
};

class TextureOpenGL : public Texture {
 public:
  static TextureOpenGLDesc GetOpenGLDesc(TextureFormat format) {
    TextureOpenGLDesc ret{};

    switch (format) {
      case TextureFormat_RGBA8: {
        ret.internalformat = GL_RGBA;
        ret.format = GL_RGBA;
        ret.type = GL_UNSIGNED_BYTE;
        break;
      }
      case TextureFormat_DEPTH: {
        ret.internalformat = GL_DEPTH_COMPONENT;
        ret.format = GL_DEPTH_COMPONENT;
        ret.type = GL_FLOAT;
        break;
      }
    }

    return ret;
  }
};

class Texture2DOpenGL : public TextureOpenGL {
 public:
  explicit Texture2DOpenGL(const TextureDesc &desc) {
    assert(desc.type == TextureType_2D);

    type = TextureType_2D;
    format = desc.format;
    multi_sample = desc.multi_sample;
    target_ = multi_sample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

    gl_desc_ = GetOpenGLDesc(format);
    GL_CHECK(glGenTextures(1, &texId_));
  }

  ~Texture2DOpenGL() {
    GL_CHECK(glDeleteTextures(1, &texId_));
  }

  int GetId() const override {
    return (int) texId_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    if (multi_sample) {
      return;
    }

    auto &sampler_2d = dynamic_cast<Sampler2DDesc &>(sampler);
    GL_CHECK(glBindTexture(target_, texId_));
    GL_CHECK(glTexParameteri(target_, GL_TEXTURE_WRAP_S, OpenGL::ConvertWrap(sampler_2d.wrap_s)));
    GL_CHECK(glTexParameteri(target_, GL_TEXTURE_WRAP_T, OpenGL::ConvertWrap(sampler_2d.wrap_t)));
    GL_CHECK(glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, OpenGL::ConvertFilter(sampler_2d.filter_min)));
    GL_CHECK(glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, OpenGL::ConvertFilter(sampler_2d.filter_mag)));

    use_mipmaps = sampler_2d.use_mipmaps;
  }

  void SetImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override {
    if (multi_sample) {
      LOGE("SetImageData not support: multi sample texture");
      return;
    }

    if (format != TextureFormat_RGBA8) {
      LOGE("SetImageData error: format not match");
      return;
    }

    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();

    GL_CHECK(glBindTexture(target_, texId_));
    GL_CHECK(glTexImage2D(target_, 0, gl_desc_.internalformat, width, height, 0, gl_desc_.format, gl_desc_.type,
                          buffers[0]->GetRawDataPtr()));

    if (use_mipmaps) {
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
      GL_CHECK(glTexImage2DMultisample(target_, 4, gl_desc_.internalformat, w, h, GL_TRUE));
    } else {
      GL_CHECK(glTexImage2D(target_, 0, gl_desc_.internalformat, w, h, 0, gl_desc_.format, gl_desc_.type, nullptr));

      GL_CHECK(glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
      GL_CHECK(glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
      GL_CHECK(glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_REPEAT));
      GL_CHECK(glTexParameteri(target_, GL_TEXTURE_WRAP_T, GL_REPEAT));

      if (use_mipmaps) {
        GL_CHECK(glGenerateMipmap(target_));
      }
    }
  }

 private:
  GLuint texId_ = 0;
  GLenum target_ = 0;
  bool use_mipmaps = false;
  TextureOpenGLDesc gl_desc_{};
};

class TextureCubeOpenGL : public TextureOpenGL {
 public:
  explicit TextureCubeOpenGL(const TextureDesc &desc) {
    assert(desc.type == TextureType_CUBE);

    type = TextureType_CUBE;
    format = desc.format;
    multi_sample = desc.multi_sample;

    gl_desc_ = GetOpenGLDesc(format);
    GL_CHECK(glGenTextures(1, &texId_));
  }

  ~TextureCubeOpenGL() {
    GL_CHECK(glDeleteTextures(1, &texId_));
  }

  int GetId() const override {
    return (int) texId_;
  }

  void SetSamplerDesc(SamplerDesc &sampler) override {
    if (multi_sample) {
      return;
    }

    auto &sampler_cube = dynamic_cast<SamplerCubeDesc &>(sampler);
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, texId_));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, OpenGL::ConvertWrap(sampler_cube.wrap_s)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, OpenGL::ConvertWrap(sampler_cube.wrap_t)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, OpenGL::ConvertWrap(sampler_cube.wrap_r)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                             OpenGL::ConvertFilter(sampler_cube.filter_min)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                             OpenGL::ConvertFilter(sampler_cube.filter_mag)));

    use_mipmaps = sampler_cube.use_mipmaps;
  }

  void SetImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override {
    if (multi_sample) {
      return;
    }

    if (format != TextureFormat_RGBA8) {
      LOGE("SetImageData error: format not match");
      return;
    }

    width = (int) buffers[0]->GetWidth();
    height = (int) buffers[0]->GetHeight();

    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, texId_));
    for (int i = 0; i < 6; i++) {
      GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, gl_desc_.internalformat, width, height, 0,
                            gl_desc_.format, gl_desc_.type, buffers[i]->GetRawDataPtr()));
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
      GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, gl_desc_.internalformat, w, h, 0,
                            gl_desc_.format, gl_desc_.type, nullptr));
    }

    if (use_mipmaps) {
      GL_CHECK(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
    }
  }

 private:
  GLuint texId_ = 0;
  bool use_mipmaps = false;
  TextureOpenGLDesc gl_desc_{};
};

}
