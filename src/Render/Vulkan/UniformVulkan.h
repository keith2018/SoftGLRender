#pragma once

#include "Base/Logger.h"
#include "Render/Uniform.h"
#include "Render/Vulkan/VKContext.h"

namespace SoftGL {

class UniformBlockVulkan : public UniformBlock {
 public:
  UniformBlockVulkan(VKContext &ctx, const std::string &name, int size)
      : vkCtx_(ctx), UniformBlock(name, size) {
    device_ = ctx.getDevice();
  }

  int getLocation(ShaderProgram &program) override {
    return 0;
  }

  void bindProgram(ShaderProgram &program, int location) override {
  }

  void setSubData(void *data, int len, int offset) override {
  }

  void setData(void *data, int len) override {
  }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;
};

class UniformSamplerVulkan : public UniformSampler {
 public:
  UniformSamplerVulkan(VKContext &ctx, const std::string &name, TextureType type, TextureFormat format)
      : vkCtx_(ctx), UniformSampler(name, type, format) {
    device_ = ctx.getDevice();
  }

  int getLocation(ShaderProgram &program) override {
    return 0;
  }

  void bindProgram(ShaderProgram &program, int location) override {
  }

  void setTexture(const std::shared_ptr<Texture> &tex) override {
  }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;
};

}
