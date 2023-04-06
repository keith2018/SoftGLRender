#pragma once

#include "Base/Logger.h"
#include "Render/Uniform.h"
#include "VKContext.h"
#include "ShaderProgramVulkan.h"

namespace SoftGL {

class UniformBlockVulkan : public UniformBlock {
 public:
  UniformBlockVulkan(VKContext &ctx, const std::string &name, int size)
      : vkCtx_(ctx), UniformBlock(name, size) {
    device_ = ctx.device();

    descriptorBufferInfo_.offset = 0;
    descriptorBufferInfo_.range = size;
  }

  int getLocation(ShaderProgram &program) override {
    auto programVulkan = dynamic_cast<ShaderProgramVulkan *>(&program);
    return programVulkan->getUniformLocation(name);
  }

  void bindProgram(ShaderProgram &program, int location) override {
    auto programVulkan = dynamic_cast<ShaderProgramVulkan *>(&program);

    if (ubo_ == nullptr) {
      LOGE("uniform bind program error: data not set");
      return;
    }

    auto *cmd = programVulkan->getCommandBuffer();
    if (cmd == nullptr) {
      LOGE("uniform bind program error: not in render pass scope");
      return;
    }
    cmd->uniformBuffers.push_back(ubo_);
    programVulkan->bindUniformBuffer(descriptorBufferInfo_, location);
    bindToCmd_ = true;
  }

  void setSubData(void *data, int len, int offset) override {
    if (bindToCmd_) {
      ubo_ = vkCtx_.getNewUniformBuffer(blockSize);
      descriptorBufferInfo_.buffer = ubo_->buffer.buffer;
    }
    memcpy((uint8_t *) ubo_->mapPtr + offset, data, len);
    bindToCmd_ = false;
  }

  void setData(void *data, int len) override {
    setSubData(data, len, 0);
  }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  bool bindToCmd_ = true;
  UniformBuffer *ubo_ = nullptr;
  VkDescriptorBufferInfo descriptorBufferInfo_{};
};

class UniformSamplerVulkan : public UniformSampler {
 public:
  UniformSamplerVulkan(VKContext &ctx, const std::string &name, TextureType type, TextureFormat format)
      : vkCtx_(ctx), UniformSampler(name, type, format) {
    device_ = ctx.device();
    descriptorImageInfo_.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  int getLocation(ShaderProgram &program) override {
    auto programVulkan = dynamic_cast<ShaderProgramVulkan *>(&program);
    return programVulkan->getUniformLocation(name);
  }

  void bindProgram(ShaderProgram &program, int location) override {
    auto programVulkan = dynamic_cast<ShaderProgramVulkan *>(&program);
    programVulkan->bindUniformSampler(descriptorImageInfo_, location);
  }

  void setTexture(const std::shared_ptr<Texture> &tex) override {
    auto *texVulkan = dynamic_cast<TextureVulkan *>(tex.get());
    descriptorImageInfo_.imageView = texVulkan->getSampleImageView();
    descriptorImageInfo_.sampler = texVulkan->getSampler();
  }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkDescriptorImageInfo descriptorImageInfo_{};
};

}
