/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/logger.h"
#include "render/uniform.h"
#include "render/soft/shader_program_soft.h"

namespace SoftGL {

class UniformBlockSoft : public UniformBlock {
 public:
  UniformBlockSoft(const std::string &name, int size) : UniformBlock(name, size) {
    buffer_.resize(size);
  }

  ~UniformBlockSoft() = default;

  int GetLocation(ShaderProgram &program) override {
    auto program_soft = dynamic_cast<ShaderProgramSoft *>(&program);
    return program_soft->GetUniformLocation(name);
  }

  void BindProgram(ShaderProgram &program, int location) override {
    auto program_soft = dynamic_cast<ShaderProgramSoft *>(&program);
    program_soft->BindUniformBlockBuffer(buffer_.data(), buffer_.size(), location);
  }

  void SetSubData(void *data, int len, int offset) override {
    memcpy(buffer_.data() + offset, data, len);
  }

  void SetData(void *data, int len) override {
    memcpy(buffer_.data(), data, len);
  }

 private:
  std::vector<uint8_t> buffer_;
};

class UniformSamplerSoft : public UniformSampler {
 public:
  explicit UniformSamplerSoft(const std::string &name, TextureType type)
      : UniformSampler(name, type) {
    switch (type) {
      case TextureType_2D:
        sampler_ = std::make_shared<Sampler2DSoft>();
        break;
      case TextureType_CUBE:
        sampler_ = std::make_shared<SamplerCubeSoft>();
        break;
      default:
        sampler_ = nullptr;
        break;
    }
  }

  ~UniformSamplerSoft() = default;

  int GetLocation(ShaderProgram &program) override {
    auto program_soft = dynamic_cast<ShaderProgramSoft *>(&program);
    return program_soft->GetUniformLocation(name);
  }

  void BindProgram(ShaderProgram &program, int location) override {
    auto program_soft = dynamic_cast<ShaderProgramSoft *>(&program);
    program_soft->BindUniformSampler(sampler_, location);
  }

  void SetTexture(const std::shared_ptr<Texture> &tex) override {
    sampler_->SetTexture(tex);
  }

 private:
  std::shared_ptr<SamplerSoft> sampler_;
};

}
