/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "Base/ImageUtils.h"
#include "Render/Texture.h"
#include "Render/OpenGL/EnumsOpenGL.h"
#include "Render/OpenGL/OpenGLUtils.h"

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
      case TextureFormat_FLOAT32: {
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

    width = desc.width;
    height = desc.height;
    type = TextureType_2D;
    format = desc.format;
    multiSample = desc.multiSample;
    target_ = multiSample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

    glDesc_ = GetOpenGLDesc(format);
    GL_CHECK(glGenTextures(1, &texId_));
  }

  ~Texture2DOpenGL() {
    GL_CHECK(glDeleteTextures(1, &texId_));
  }

  int getId() const override {
    return (int) texId_;
  }

  void setSamplerDesc(SamplerDesc &sampler) override {
    if (multiSample) {
      return;
    }

    auto &sampler2d = dynamic_cast<Sampler2DDesc &>(sampler);
    GL_CHECK(glBindTexture(target_, texId_));
    GL_CHECK(glTexParameteri(target_, GL_TEXTURE_WRAP_S, OpenGL::cvtWrap(sampler2d.wrapS)));
    GL_CHECK(glTexParameteri(target_, GL_TEXTURE_WRAP_T, OpenGL::cvtWrap(sampler2d.wrapT)));
    GL_CHECK(glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, OpenGL::cvtFilter(sampler2d.filterMin)));
    GL_CHECK(glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, OpenGL::cvtFilter(sampler2d.filterMag)));
    GL_CHECK(glTexParameterfv(target_, GL_TEXTURE_BORDER_COLOR, &sampler2d.borderColor[0]));

    useMipmaps = sampler2d.useMipmaps;
  }

  void setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override {
    if (multiSample) {
      LOGE("setImageData not support: multi sample texture");
      return;
    }

    if (format != TextureFormat_RGBA8) {
      LOGE("setImageData error: format not match");
      return;
    }

    if (width != buffers[0]->getWidth() || height != buffers[0]->getHeight()) {
      LOGE("setImageData error: size not match");
      return;
    }

    GL_CHECK(glBindTexture(target_, texId_));
    GL_CHECK(glTexImage2D(target_, 0, glDesc_.internalformat, width, height, 0, glDesc_.format, glDesc_.type,
                          buffers[0]->getRawDataPtr()));

    if (useMipmaps) {
      GL_CHECK(glGenerateMipmap(target_));
    }
  }

  void initImageData() override {
    GL_CHECK(glBindTexture(target_, texId_));
    if (multiSample) {
      GL_CHECK(glTexImage2DMultisample(target_, 4, glDesc_.internalformat, width, height, GL_TRUE));
    } else {
      GL_CHECK(glTexImage2D(target_, 0, glDesc_.internalformat, width, height, 0, glDesc_.format, glDesc_.type,
                            nullptr));

      if (useMipmaps) {
        GL_CHECK(glGenerateMipmap(target_));
      }
    }
  }

  void dumpImage(const char *path) override {
    if (multiSample) {
      return;
    }

    GLuint fbo;
    GL_CHECK(glGenFramebuffers(1, &fbo));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

    GLenum attachment = format == TextureFormat_FLOAT32 ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0;
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texId_, 0));

    auto *pixels = new uint8_t[width * height * 4];
    GL_CHECK(glReadPixels(0, 0, width, height, glDesc_.format, glDesc_.type, pixels));

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GL_CHECK(glDeleteFramebuffers(1, &fbo));

    // convert float to rgba
    if (format == TextureFormat_FLOAT32) {
      ImageUtils::convertFloatImage(reinterpret_cast<RGBA *>(pixels), reinterpret_cast<float *>(pixels),
                                    width, height);
    }
    ImageUtils::writeImage(path, width, height, 4, pixels, width * 4, true);
    delete[] pixels;
  }

 private:
  GLuint texId_ = 0;
  GLenum target_ = 0;
  bool useMipmaps = false;
  TextureOpenGLDesc glDesc_{};
};

class TextureCubeOpenGL : public TextureOpenGL {
 public:
  explicit TextureCubeOpenGL(const TextureDesc &desc) {
    assert(desc.type == TextureType_CUBE);

    width = desc.width;
    height = desc.height;
    type = TextureType_CUBE;
    format = desc.format;
    multiSample = desc.multiSample;

    glDesc_ = GetOpenGLDesc(format);
    GL_CHECK(glGenTextures(1, &texId_));
  }

  ~TextureCubeOpenGL() {
    GL_CHECK(glDeleteTextures(1, &texId_));
  }

  int getId() const override {
    return (int) texId_;
  }

  void setSamplerDesc(SamplerDesc &sampler) override {
    if (multiSample) {
      return;
    }

    auto &samplerCube = dynamic_cast<SamplerCubeDesc &>(sampler);
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, texId_));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, OpenGL::cvtWrap(samplerCube.wrapS)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, OpenGL::cvtWrap(samplerCube.wrapT)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, OpenGL::cvtWrap(samplerCube.wrapR)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                             OpenGL::cvtFilter(samplerCube.filterMin)));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                             OpenGL::cvtFilter(samplerCube.filterMag)));
    GL_CHECK(glTexParameterfv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, &samplerCube.borderColor[0]));

    useMipmaps = samplerCube.useMipmaps;
  }

  void setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override {
    if (multiSample) {
      return;
    }

    if (format != TextureFormat_RGBA8) {
      LOGE("setImageData error: format not match");
      return;
    }

    if (width != buffers[0]->getWidth() || height != buffers[0]->getHeight()) {
      LOGE("setImageData error: size not match");
      return;
    }

    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, texId_));
    for (int i = 0; i < 6; i++) {
      GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glDesc_.internalformat, width, height, 0,
                            glDesc_.format, glDesc_.type, buffers[i]->getRawDataPtr()));
    }
    if (useMipmaps) {
      GL_CHECK(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
    }
  }

  void initImageData() override {
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, texId_));
    for (int i = 0; i < 6; i++) {
      GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glDesc_.internalformat, width, height, 0,
                            glDesc_.format, glDesc_.type, nullptr));
    }

    if (useMipmaps) {
      GL_CHECK(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
    }
  }

 private:
  GLuint texId_ = 0;
  bool useMipmaps = false;
  TextureOpenGLDesc glDesc_{};
};

}
