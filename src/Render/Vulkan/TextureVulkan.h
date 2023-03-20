/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Texture.h"
#include "VKContext.h"
#include "EnumsVulkan.h"

namespace SoftGL {

class TextureVulkan : public Texture {
 public:
  TextureVulkan(VKContext &ctx, const TextureDesc &desc) : vkCtx_(ctx) {
    device_ = ctx.device();

    width = desc.width;
    height = desc.height;
    type = desc.type;
    format = desc.format;
    multiSample = desc.multiSample;
  }

  virtual ~TextureVulkan() {
    vkDestroyImageView(device_, view_, nullptr);
    vkDestroyImage(device_, image_, nullptr);
    vkFreeMemory(device_, memory_, nullptr);
  }

  int getId() const override {
    return uuid_.get();
  }

  void initImageData() override {
    createImage();
    createMemory();
  };

  void createImageView(VkImageAspectFlagBits aspectMask) {
    if (view_ != VK_NULL_HANDLE) {
      vkDestroyImageView(device_, view_, nullptr);
      view_ = VK_NULL_HANDLE;
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
    VK_CHECK(vkCreateImageView(device_, &imageViewCreateInfo, nullptr, &view_));
  }

 protected:
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

    VK_CHECK(vkCreateImage(device_, &imageInfo, nullptr, &image_));
  }

  void createMemory() {
    if (memory_ != VK_NULL_HANDLE) {
      return;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device_, image_, &memReqs);

    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = vkCtx_.getMemoryTypeIndex(memReqs.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(device_, &memAllocInfo, nullptr, &memory_));
    VK_CHECK(vkBindImageMemory(device_, image_, memory_, 0));
  }

 public:
  VkImage image_ = VK_NULL_HANDLE;
  VkDeviceMemory memory_ = VK_NULL_HANDLE;
  VkImageView view_ = VK_NULL_HANDLE;

 protected:
  UUID<TextureVulkan> uuid_;
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;
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
