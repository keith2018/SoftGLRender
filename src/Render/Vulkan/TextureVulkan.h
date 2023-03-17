/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Texture.h"
#include "EnumsVulkan.h"

namespace SoftGL {

class TextureVulkan : public Texture {
 public:
  TextureVulkan(VKContext &ctx, const TextureDesc &desc) : vkCtx_(ctx) {
    width = desc.width;
    height = desc.height;
    type = desc.type;
    format = desc.format;
    multiSample = desc.multiSample;
  }

  int getId() const override {
    return uuid_.get();
  }

  void setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override {

  };

  void setImageData(const std::vector<std::shared_ptr<Buffer<float>>> &buffers) override {

  };

  void initImageData() override {

  };

  void dumpImage(const char *path) override {

  };

 private:
  void createImage() {
    if (image_ != VK_NULL_HANDLE) {
      return;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK::cvtImageType(type);
    imageInfo.format = VK::cvtImageFormat(format);
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VK_CHECK(vkCreateImage(vkCtx_.device, &imageInfo, nullptr, &image_));
  }

  void createMemory() {
    if (memory_ != VK_NULL_HANDLE) {
      return;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vkCtx_.device, image_, &memReqs);

    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = VK::getMemoryTypeIndex(vkCtx_.physicalDevice, memReqs.memoryTypeBits,
                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(vkCtx_.device, &memAllocInfo, nullptr, &memory_));
    VK_CHECK(vkBindImageMemory(vkCtx_.device, image_, memory_, 0));
  }

  void createImageView(VkImageAspectFlagBits aspectMask) {
    if (view_ != VK_NULL_HANDLE) {
      return;
    }

    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.viewType = VK::cvtImageViewType(type);
    imageViewCreateInfo.format = VK::cvtImageFormat(format);
    imageViewCreateInfo.subresourceRange = {};
    imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.image = image_;
    VK_CHECK(vkCreateImageView(vkCtx_.device, &imageViewCreateInfo, nullptr, &view_));
  }

 private:
  UUID<TextureVulkan> uuid_;

  VKContext &vkCtx_;
  VkImage image_ = VK_NULL_HANDLE;
  VkDeviceMemory memory_ = VK_NULL_HANDLE;
  VkImageView view_ = VK_NULL_HANDLE;
};

class Texture2DVulkan : public TextureVulkan {
 public:
  Texture2DVulkan(VKContext &ctx, const TextureDesc &desc)
      : TextureVulkan(ctx, desc) {}

  void setSamplerDesc(SamplerDesc &sampler) override {
    samplerDesc_ = dynamic_cast<Sampler2DDesc &>(sampler);
  };

 private:
  Sampler2DDesc samplerDesc_;
};

class TextureCubeVulkan : public TextureVulkan {
 public:
  TextureCubeVulkan(VKContext &ctx, const TextureDesc &desc)
      : TextureVulkan(ctx, desc) {}

  void setSamplerDesc(SamplerDesc &sampler) override {
    samplerDesc_ = dynamic_cast<SamplerCubeDesc &>(sampler);
  };

 private:
  SamplerCubeDesc samplerDesc_;
};

}
