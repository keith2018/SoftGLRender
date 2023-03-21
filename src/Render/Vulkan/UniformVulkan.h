#pragma once

#include "Base/Logger.h"
#include "Render/Uniform.h"
#include "Render/Vulkan/VKContext.h"

namespace SoftGL {

class UniformBlockVulkan : public UniformBlock {
 public:
  UniformBlockVulkan(VKContext &ctx, const std::string &name, int size)
      : vkCtx_(ctx), UniformBlock(name, size) {
    device_ = ctx.device();

    VkDeviceSize bufferSize = size;
    VulkanUtils::createBuffer(vkCtx_, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              buffer_, memory_);
    VK_CHECK(vkMapMemory(vkCtx_.device(), memory_, 0, bufferSize, 0, &buffersMapped_));
  }

  ~UniformBlockVulkan() {
    vkDestroyBuffer(vkCtx_.device(), buffer_, nullptr);
    vkUnmapMemory(vkCtx_.device(), memory_);
    vkFreeMemory(vkCtx_.device(), memory_, nullptr);
  }

  int getLocation(ShaderProgram &program) override {
    return 0;
  }

  void bindProgram(ShaderProgram &program, int location) override {
  }

  void setSubData(void *data, int len, int offset) override {
    memcpy((uint8_t *) buffersMapped_ + offset, data, len);
  }

  void setData(void *data, int len) override {
    memcpy(buffersMapped_, data, len);
  }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkBuffer buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory memory_ = VK_NULL_HANDLE;
  void *buffersMapped_ = nullptr;
};

class UniformSamplerVulkan : public UniformSampler {
 public:
  UniformSamplerVulkan(VKContext &ctx, const std::string &name, TextureType type, TextureFormat format)
      : vkCtx_(ctx), UniformSampler(name, type, format) {
    device_ = ctx.device();
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
