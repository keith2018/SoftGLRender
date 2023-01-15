#pragma once

#include <glad/glad.h>
#include "base/logger.h"
#include "render/uniform.h"
#include "render/opengl/shader_program_opengl.h"
#include "render/opengl/opengl_utils.h"


namespace SoftGL {

class UniformBlockOpenGL : public UniformBlock {
 public:
  UniformBlockOpenGL(const std::string &name, int size) : UniformBlock(name, size) {
    GL_CHECK(glGenBuffers(1, &ubo_));
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, ubo_));
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STATIC_DRAW));
  }

  ~UniformBlockOpenGL() {
    GL_CHECK(glDeleteBuffers(1, &ubo_));
  }

  int GetLocation(ShaderProgram &program) override {
    return glGetUniformBlockIndex(program.GetId(), name.c_str());
  }

  void BindProgram(ShaderProgram &program, int location, int binding) override {
    if (location < 0) {
      return;
    }
    GL_CHECK(glUniformBlockBinding(program.GetId(), location, binding));
    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo_));
  }

  void SetSubData(void *data, int len, int offset) override {
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, ubo_));
    GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, offset, len, data));
  }

  void SetData(void *data, int len) override {
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, ubo_));
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, len, data, GL_STATIC_DRAW));
  }

 private:
  GLuint ubo_ = 0;
};

class UniformSamplerOpenGL : public UniformSampler {
 public:
  explicit UniformSamplerOpenGL(const std::string &name) : UniformSampler(name) {}
  ~UniformSamplerOpenGL() = default;

  int GetLocation(ShaderProgram &program) override {
    return glGetUniformLocation(program.GetId(), name.c_str());
  }

  void BindProgram(ShaderProgram &program, int location, int binding) override {
    if (location < 0) {
      return;
    }

#define BIND_TEX(n) case n: GL_CHECK(glActiveTexture(GL_TEXTURE##n)); break;
    switch (binding) {
      BIND_TEX(0)
      BIND_TEX(1)
      BIND_TEX(2)
      BIND_TEX(3)
      BIND_TEX(4)
      BIND_TEX(5)
      BIND_TEX(6)
      BIND_TEX(7)
      default: {
        LOGE("UniformSampler::BindProgram error: texture unit not support");
        break;
      }
    }
    GL_CHECK(glBindTexture(texTarget_, texId_));
    GL_CHECK(glUniform1i(location, binding));
  }

  void SetTexture(const std::shared_ptr<Texture> &tex) override {
    switch (tex->Type()) {
      case TextureType_2D:
        texTarget_ = GL_TEXTURE_2D;
        break;
      case TextureType_CUBE:
        texTarget_ = GL_TEXTURE_CUBE_MAP;
        break;
      default:
        LOGE("UniformSampler::SetTexture error: texture type not support");
        break;
    }
    texId_ = tex->GetId();
  }

 private:
  GLuint texTarget_ = 0;
  GLuint texId_ = 0;
};

}
