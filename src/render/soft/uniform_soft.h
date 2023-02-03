/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/logger.h"
#include "render/uniform.h"

namespace SoftGL {

class UniformBlockSoft : public UniformBlock {
 public:
  UniformBlockSoft(const std::string &name, int size) : UniformBlock(name, size) {
  }

  ~UniformBlockSoft() = default;

  int GetLocation(ShaderProgram &program) override {
  }

  void BindProgram(ShaderProgram &program, int location, int binding) override {

  }

  void SetSubData(void *data, int len, int offset) override {

  }

  void SetData(void *data, int len) override {
  }

 private:
};

class UniformSamplerSoft : public UniformSampler {
 public:
  explicit UniformSamplerSoft(const std::string &name) : UniformSampler(name) {}

  ~UniformSamplerSoft() = default;

  int GetLocation(ShaderProgram &program) override {
  }

  void BindProgram(ShaderProgram &program, int location, int binding) override {
  }

  void SetTexture(const std::shared_ptr<Texture> &tex) override {
  }

 private:
};

}
