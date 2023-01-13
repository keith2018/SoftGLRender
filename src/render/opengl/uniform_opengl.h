#pragma once

#include <glad/glad.h>
#include "base/logger.h"
#include "render/uniform.h"
#include "render/opengl/shader_program_opengl.h"

namespace SoftGL {

class UniformBlockOpenGL : public UniformBlock {
 public:
  UniformBlockOpenGL(const std::string &name, int size) : UniformBlock(name, size) {
    glGenBuffers(1, &ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STATIC_DRAW);
  }

  ~UniformBlockOpenGL() {
    glDeleteBuffers(1, &ubo_);
  }

  int GetLocation(ShaderProgram &program) override {
    auto &program_gl = dynamic_cast<ShaderProgramOpenGL &>(program);
    return glGetUniformBlockIndex(program_gl.GetId(), name.c_str());
  }

  void BindProgram(ShaderProgram &program, int location, int binding) override {
    if (location < 0) {
      return;
    }
    auto &program_gl = dynamic_cast<ShaderProgramOpenGL &>(program);
    glUniformBlockBinding(program_gl.GetId(), location, binding);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo_);
  }

  void SetSubData(void *data, int len, int offset) override {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, len, data);
  }

  void SetData(void *data, int len) override {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, len, data, GL_STATIC_DRAW);
  }

 private:
  GLuint ubo_ = 0;
};

class UniformSamplerOpenGL : public UniformSampler {
 public:
  explicit UniformSamplerOpenGL(const std::string &name) : UniformSampler(name) {}
  ~UniformSamplerOpenGL() = default;

  int GetLocation(ShaderProgram &program) override {
    auto &program_gl = dynamic_cast<ShaderProgramOpenGL &>(program);
    return glGetUniformLocation(program_gl.GetId(), name.c_str());
  }

  void BindProgram(ShaderProgram &program, int location, int binding) override {
    if (location < 0) {
      return;
    }

#define BIND_TEX(n) case n: glActiveTexture(GL_TEXTURE##n); break;
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
        LOGE("texture unit not support");
        break;
      }
    }
    glBindTexture(texTarget_, texId_);
    glUniform1i(location, binding);
  }

  void SetTexture2D(Texture2D &tex) override {
    auto &tex_gl = dynamic_cast<Texture2DOpenGL &>(tex);
    texTarget_ = GL_TEXTURE_2D;
    texId_ = tex_gl.GetId();
  }

  void SetTextureCube(TextureCube &tex) override {
    auto &tex_gl = dynamic_cast<TextureCubeOpenGL &>(tex);
    texTarget_ = GL_TEXTURE_CUBE_MAP;
    texId_ = tex_gl.GetId();
  }

 private:
  GLuint texTarget_ = 0;
  GLuint texId_ = 0;
};

}
