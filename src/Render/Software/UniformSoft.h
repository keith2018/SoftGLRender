/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/Logger.h"
#include "Render/Uniform.h"
#include "Render/Software/ShaderProgramSoft.h"

namespace SoftGL {

class UniformBlockSoft : public UniformBlock {
 public:
  UniformBlockSoft(const std::string &name, int size) : UniformBlock(name, size) {
    buffer_.resize(size);
  }

  int getBinding(ShaderProgram &program) override {
    auto programSoft = dynamic_cast<ShaderProgramSoft *>(&program);
    return programSoft->getUniformLocation(name);
  }

  void bindProgram(ShaderProgram &program, int binding) override {
    auto programSoft = dynamic_cast<ShaderProgramSoft *>(&program);
    programSoft->bindUniformBlockBuffer(buffer_.data(), buffer_.size(), binding);
  }

  void setSubData(void *data, int len, int offset) override {
    memcpy(buffer_.data() + offset, data, len);
  }

  void setData(void *data, int len) override {
    memcpy(buffer_.data(), data, len);
  }

 private:
  std::vector<uint8_t> buffer_;
};

class UniformSamplerSoft : public UniformSampler {
 public:
  explicit UniformSamplerSoft(const std::string &name, TextureType type, TextureFormat format)
      : UniformSampler(name, type, format) {
    switch (type) {
      case TextureType_2D:
        switch (format) {
          case TextureFormat_RGBA8:
            sampler_ = std::make_shared<Sampler2DSoft<RGBA>>();
            break;
          case TextureFormat_FLOAT32:
            sampler_ = std::make_shared<Sampler2DSoft<float>>();
            break;
        }
        break;
      case TextureType_CUBE:
        switch (format) {
          case TextureFormat_RGBA8:
            sampler_ = std::make_shared<SamplerCubeSoft<RGBA>>();
            break;
          case TextureFormat_FLOAT32:
            sampler_ = std::make_shared<SamplerCubeSoft<float>>();
            break;
        }
        break;
      default:
        sampler_ = nullptr;
        break;
    }
  }

  int getBinding(ShaderProgram &program) override {
    auto programSoft = dynamic_cast<ShaderProgramSoft *>(&program);
    return programSoft->getUniformLocation(name);
  }

  void bindProgram(ShaderProgram &program, int binding) override {
    auto programSoft = dynamic_cast<ShaderProgramSoft *>(&program);
    programSoft->bindUniformSampler(sampler_, binding);
  }

  void setTexture(const std::shared_ptr<Texture> &tex) override {
    sampler_->setTexture(tex);
  }

 private:
  std::shared_ptr<SamplerSoft> sampler_;
};

}
