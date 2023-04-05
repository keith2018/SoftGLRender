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

    VkDeviceSize bufferSize = size;
    vkCtx_.createBuffer(buffer_, bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkCtx_.createStagingBuffer(stagingBuffer_, bufferSize);

    descriptorBufferInfo_.buffer = buffer_.buffer;
    descriptorBufferInfo_.offset = 0;
    descriptorBufferInfo_.range = bufferSize;

    VK_CHECK(vkMapMemory(device_, stagingBuffer_.memory, 0, bufferSize, 0, &buffersMapped_));
  }

  ~UniformBlockVulkan() {
    vkUnmapMemory(device_, stagingBuffer_.memory);

    vkDestroyBuffer(device_, stagingBuffer_.buffer, nullptr);
    vkFreeMemory(device_, stagingBuffer_.memory, nullptr);

    vkDestroyBuffer(device_, buffer_.buffer, nullptr);
    vkFreeMemory(device_, buffer_.memory, nullptr);
  }

  int getLocation(ShaderProgram &program) override {
    auto programVulkan = dynamic_cast<ShaderProgramVulkan *>(&program);
    return programVulkan->getUniformLocation(name);
  }

  void bindProgram(ShaderProgram &program, int location) override {
    auto programVulkan = dynamic_cast<ShaderProgramVulkan *>(&program);
    programVulkan->bindUniformBuffer(descriptorBufferInfo_, location);
  }

  void setSubData(void *data, int len, int offset) override {
    uploadBufferData(data, len, offset);
  }

  void setData(void *data, int len) override {
    uploadBufferData(data, len);
  }

 private:
  void uploadBufferData(void *bufferData, int bufferSize, int offset = 0) {
    memcpy((uint8_t *) buffersMapped_ + offset, bufferData, bufferSize);

    auto &commandBuffer = vkCtx_.beginCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(commandBuffer.cmdBuffer, stagingBuffer_.buffer, buffer_.buffer, 1, &copyRegion);

    // use barrier to ensure that data is uploaded to the GPU before it is accessed
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_UNIFORM_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buffer_.buffer;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(commandBuffer.cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

    vkCtx_.endCommands(commandBuffer);
  }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  AllocatedBuffer buffer_{};
  AllocatedBuffer stagingBuffer_{};

  void *buffersMapped_ = nullptr;
  VkDescriptorBufferInfo descriptorBufferInfo_{};
};

class UniformSamplerVulkan : public UniformSampler {
 public:
  UniformSamplerVulkan(VKContext &ctx, const std::string &name, TextureType type, TextureFormat format)
      : vkCtx_(ctx), UniformSampler(name, type, format) {
    device_ = ctx.device();
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
    descriptorImageInfo_.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo_.imageView = texVulkan->getSampleImageView();
    descriptorImageInfo_.sampler = texVulkan->getSampler();
  }

 private:
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkDescriptorImageInfo descriptorImageInfo_{};
};

}
