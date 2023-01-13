/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <glad/glad.h>
#include "render/texture.h"
#include "enums_opengl.h"

namespace SoftGL {

class Texture2DOpenGL : public Texture2D {
 public:
  explicit Texture2DOpenGL(int refId) : ref_(true) {
    texId_ = refId;
  }

  Texture2DOpenGL() : ref_(false) {
    glGenTextures(1, &texId_);
  }

  ~Texture2DOpenGL() {
    if (ref_) {
      return;
    }
    glDeleteTextures(1, &texId_);
  }

  inline GLuint GetId() const {
    return texId_;
  }

  void SetSampler(Sampler2D &sampler) override {
    glBindTexture(GL_TEXTURE_2D, texId_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, OpenGL::ConvertWrap(sampler.wrap_s));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, OpenGL::ConvertWrap(sampler.wrap_t));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OpenGL::ConvertFilter(sampler.filter_min));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OpenGL::ConvertFilter(sampler.filter_mag));

    use_mipmaps = sampler.use_mipmaps;
  }

  void SetImageData(BufferRGBA &buffer) override {
    width = (int) buffer.GetWidth();
    height = (int) buffer.GetHeight();
    glBindTexture(GL_TEXTURE_2D, texId_);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 buffer.GetRawDataPtr());
    if (use_mipmaps) {
      glGenerateMipmap(GL_TEXTURE_2D);
    }
  }

  void InitImageData(int w, int h) override {
    width = w;
    height = h;
    glBindTexture(GL_TEXTURE_2D, texId_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }

 private:
  bool ref_ = false;
  GLuint texId_ = 0;
};

class TextureCubeOpenGL : public TextureCube {
 public:
  TextureCubeOpenGL() {
    glGenTextures(1, &texId_);
  }

  ~TextureCubeOpenGL() {
    glDeleteTextures(1, &texId_);
  }

  inline GLuint GetId() const {
    return texId_;
  }

  void SetSampler(SamplerCube &sampler) override {
    glBindTexture(GL_TEXTURE_CUBE_MAP, texId_);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, OpenGL::ConvertWrap(sampler.wrap_s));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, OpenGL::ConvertWrap(sampler.wrap_t));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, OpenGL::ConvertWrap(sampler.wrap_r));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, OpenGL::ConvertFilter(sampler.filter_min));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, OpenGL::ConvertFilter(sampler.filter_mag));

    use_mipmaps = sampler.use_mipmaps;
  }

  void SetImageData(BufferRGBA &buffer, CubeMapFace face) override {
    width = (int) buffer.GetWidth();
    height = (int) buffer.GetHeight();
    GLint face_gl = OpenGL::ConvertCubeFace(face);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texId_);
    glTexImage2D(face_gl,
                 0,
                 GL_RGBA,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 buffer.GetRawDataPtr());
    if (use_mipmaps) {
      glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
  }

  void InitImageData(int w, int h, CubeMapFace face) override {
    width = w;
    height = h;
    GLint face_gl = OpenGL::ConvertCubeFace(face);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texId_);
    glTexImage2D(face_gl, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  }

 private:
  GLuint texId_ = 0;
};

class TextureDepthOpenGL : public TextureDepth {
 public:
  TextureDepthOpenGL() {
    glGenRenderbuffers(1, &rbId_);
  }

  ~TextureDepthOpenGL() {
    glDeleteRenderbuffers(1, &rbId_);
  }

  inline GLuint GetId() const {
    return rbId_;
  }

  void InitImageData(int w, int h) override {
    width = w;
    height = h;
    glBindRenderbuffer(GL_RENDERBUFFER, rbId_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  }

 private:
  GLuint rbId_ = 0;
};

}