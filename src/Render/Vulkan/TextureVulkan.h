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
    usage = desc.usage;
    useMipmaps = desc.useMipmaps;
    multiSample = desc.multiSample;
  }

  virtual ~TextureVulkan() {
    vkDestroySampler(device_, sampler_, nullptr);
    vkDestroyImageView(device_, view_, nullptr);
    vkDestroyImage(device_, image_, nullptr);
    vkFreeMemory(device_, memory_, nullptr);
  }

  int getId() const override {
    return uuid_.get();
  }
  void setSamplerDesc(SamplerDesc &sampler) override {
    samplerDesc_ = sampler;
  };

  void initImageData() override {
    createImage();
    createMemory();
  };

  inline VkImage &getVkImage() {
    return image_;
  }

  VkImageView &createImageView(VkImageAspectFlagBits aspectMask = VK_IMAGE_ASPECT_COLOR_BIT) {
    if (view_ != VK_NULL_HANDLE) {
      return view_;
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

    return view_;
  }

  VkSampler &createSampler() {
    if (sampler_ != VK_NULL_HANDLE) {
      return sampler_;
    }

    // TODO samplerDesc_
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = vkCtx_.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VK_CHECK(vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_));

    return sampler_;
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
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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

 protected:
  SamplerDesc samplerDesc_;

  VkImage image_ = VK_NULL_HANDLE;
  VkDeviceMemory memory_ = VK_NULL_HANDLE;
  VkImageView view_ = VK_NULL_HANDLE;
  VkSampler sampler_ = VK_NULL_HANDLE;

  UUID<TextureVulkan> uuid_;
  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;
};

class Texture2DVulkan : public TextureVulkan {
 public:
  Texture2DVulkan(VKContext &ctx, const TextureDesc &desc)
      : TextureVulkan(ctx, desc) {}

  void setImageData(const std::vector<std::shared_ptr<Buffer<RGBA>>> &buffers) override {
    auto &dataBuffer = buffers[0];
    VkDeviceSize imageSize = dataBuffer->getRawDataSize();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vkCtx_.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

    void *dataPtr;
    VK_CHECK(vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &dataPtr));
    memcpy(dataPtr, dataBuffer->getRawDataPtr(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device_, stagingBufferMemory);

    VkCommandBuffer copyCmd = vkCtx_.beginSingleTimeCommands();
    VKContext::transitionImageLayout(copyCmd,
                                     image_,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {(uint32_t) width, (uint32_t) height, 1};

    vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VKContext::transitionImageLayout(copyCmd,
                                     image_,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    vkCtx_.endSingleTimeCommands(copyCmd);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
  }
};

class TextureCubeVulkan : public TextureVulkan {
 public:
  TextureCubeVulkan(VKContext &ctx, const TextureDesc &desc)
      : TextureVulkan(ctx, desc) {}
};

}
