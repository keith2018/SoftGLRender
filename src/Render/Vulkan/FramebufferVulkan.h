/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Framebuffer.h"
#include "Render/Vulkan/TextureVulkan.h"
#include "VulkanUtils.h"

namespace SoftGL {

class FrameBufferVulkan : public FrameBuffer {
 public:
  explicit FrameBufferVulkan(VKContext &ctx) : vkCtx_(ctx) {}
  virtual ~FrameBufferVulkan() {
    vkDestroyFramebuffer(vkCtx_.device, framebuffer_, nullptr);
    vkDestroyImage(vkCtx_.device, hostImage_, nullptr);
    vkFreeMemory(vkCtx_.device, hostImageMemory_, nullptr);
  }

  int getId() const override {
    return uuid_.get();
  }

  bool isValid() override {
    return colorReady || depthReady;
  }

  void create(VkRenderPass renderPass) {
    if (framebuffer_ != VK_NULL_HANDLE) {
      return;
    }

    if (isValid()) {
      return;
    }

    int attachCnt = 0;
    VkImageView attachments[2];
    if (colorReady) {
      auto *texColor = dynamic_cast<Texture2DVulkan *>(colorAttachment2d.tex.get());
      texColor->createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
      attachments[attachCnt++] = texColor->view_;
      width_ = texColor->width;
      height_ = texColor->height;
    }
    if (depthReady) {
      auto *texDepth = dynamic_cast<Texture2DVulkan *>(depthAttachment.get());
      texDepth->createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
      attachments[attachCnt++] = texDepth->view_;
      width_ = texDepth->width;
      height_ = texDepth->height;
    }

    VkFramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.attachmentCount = attachCnt;
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = width_;
    framebufferCreateInfo.height = height_;
    framebufferCreateInfo.layers = 1;
    VK_CHECK(vkCreateFramebuffer(vkCtx_.device, &framebufferCreateInfo, nullptr, &framebuffer_));
  }

  void createHostImage() {
    if (hostImage_ != VK_NULL_HANDLE) {
      return;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width = width_;
    imageInfo.extent.height = height_;
    imageInfo.extent.depth = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.mipLevels = 1;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VK_CHECK(vkCreateImage(vkCtx_.device, &imageInfo, nullptr, &hostImage_));

    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    vkGetImageMemoryRequirements(vkCtx_.device, hostImage_, &memRequirements);
    memAllocInfo.allocationSize = memRequirements.size;

    memAllocInfo.memoryTypeIndex = VK::getMemoryTypeIndex(vkCtx_.physicalDevice, memRequirements.memoryTypeBits,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK(vkAllocateMemory(vkCtx_.device, &memAllocInfo, nullptr, &hostImageMemory_));
    VK_CHECK(vkBindImageMemory(vkCtx_.device, hostImage_, hostImageMemory_, 0));
  }

  void readColorPixels(const std::function<void(uint8_t *buffer, uint32_t width, uint32_t height)> &func) {
    VkImageSubresource subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout subResourceLayout;

    vkGetImageSubresourceLayout(vkCtx_.device, hostImage_, &subResource, &subResourceLayout);

    uint8_t *pixelPtr = nullptr;
    vkMapMemory(vkCtx_.device, hostImageMemory_, 0, VK_WHOLE_SIZE, 0, (void **) &pixelPtr);
    pixelPtr += subResourceLayout.offset;

    if (func) {
      func(pixelPtr, width_, height_);
    }

    vkUnmapMemory(vkCtx_.device, hostImageMemory_);
    vkQueueWaitIdle(vkCtx_.graphicsQueue);
  }

  inline VkFramebuffer getVKFramebuffer() {
    return framebuffer_;
  }

  VkImage getColorAttachmentImage() {
    if (colorReady) {
      auto *texColor = dynamic_cast<Texture2DVulkan *>(colorAttachment2d.tex.get());
      return texColor->image_;
    }
    return VK_NULL_HANDLE;
  }

  VkImage getDepthAttachmentImage() {
    if (depthReady) {
      auto *texColor = dynamic_cast<Texture2DVulkan *>(depthAttachment.get());
      return texColor->image_;
    }
    return VK_NULL_HANDLE;
  }

  inline VkImage getHostImage() {
    return hostImage_;
  }

  inline uint32_t width() const {
    return width_;
  }

  inline uint32_t height() const {
    return height_;
  }

 private:
  UUID<FrameBufferVulkan> uuid_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  VKContext &vkCtx_;
  VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

  VkImage hostImage_ = VK_NULL_HANDLE;
  VkDeviceMemory hostImageMemory_ = VK_NULL_HANDLE;
};

}
