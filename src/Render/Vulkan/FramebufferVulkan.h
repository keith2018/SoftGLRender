/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/UUID.h"
#include "Render/Framebuffer.h"
#include "Render/Vulkan/TextureVulkan.h"
#include "VKContext.h"

namespace SoftGL {

class FrameBufferVulkan : public FrameBuffer {
 public:
  explicit FrameBufferVulkan(VKContext &ctx) : vkCtx_(ctx) {
    device_ = ctx.device();
  }

  virtual ~FrameBufferVulkan() {
    vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    vkDestroyImage(device_, hostImage_, nullptr);
    vkFreeMemory(device_, hostImageMemory_, nullptr);

    vkDestroyRenderPass(device_, renderPass_, nullptr);
  }

  int getId() const override {
    return uuid_.get();
  }

  bool isValid() override {
    return colorReady || depthReady;
  }

  void create() {
    createRenderPass();
    createFramebuffer();
  }

  void readColorPixels(const std::function<void(uint8_t *buffer, uint32_t width, uint32_t height)> &func) {
    createHostImage();

    VkImageSubresource subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout subResourceLayout;

    vkGetImageSubresourceLayout(device_, hostImage_, &subResource, &subResourceLayout);

    uint8_t *pixelPtr = nullptr;
    vkMapMemory(device_, hostImageMemory_, 0, VK_WHOLE_SIZE, 0, (void **) &pixelPtr);
    pixelPtr += subResourceLayout.offset;

    if (func) {
      func(pixelPtr, width_, height_);
    }

    vkUnmapMemory(device_, hostImageMemory_);
    vkQueueWaitIdle(vkCtx_.getGraphicsQueue());
  }

  inline VkRenderPass getRenderPass() {
    return renderPass_;
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
    createHostImage();
    return hostImage_;
  }

  inline uint32_t width() const {
    return width_;
  }

  inline uint32_t height() const {
    return height_;
  }

 private:
  void createRenderPass() {
    if (renderPass_ != VK_NULL_HANDLE) {
      return;
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK::cvtImageFormat(getColorAttachmentDesc()->format);
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_));
  }

  void createFramebuffer() {
    if (framebuffer_ != VK_NULL_HANDLE) {
      return;
    }

    if (!isValid()) {
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
    framebufferCreateInfo.renderPass = renderPass_;
    framebufferCreateInfo.attachmentCount = attachCnt;
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = width_;
    framebufferCreateInfo.height = height_;
    framebufferCreateInfo.layers = 1;
    VK_CHECK(vkCreateFramebuffer(device_, &framebufferCreateInfo, nullptr, &framebuffer_));
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
    VK_CHECK(vkCreateImage(device_, &imageInfo, nullptr, &hostImage_));

    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    vkGetImageMemoryRequirements(device_, hostImage_, &memRequirements);
    memAllocInfo.allocationSize = memRequirements.size;

    memAllocInfo.memoryTypeIndex = vkCtx_.getMemoryTypeIndex(memRequirements.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                                 | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK(vkAllocateMemory(device_, &memAllocInfo, nullptr, &hostImageMemory_));
    VK_CHECK(vkBindImageMemory(device_, hostImage_, hostImageMemory_, 0));
  }

 private:
  UUID<FrameBufferVulkan> uuid_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;
  VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

  VkImage hostImage_ = VK_NULL_HANDLE;
  VkDeviceMemory hostImageMemory_ = VK_NULL_HANDLE;

  VkRenderPass renderPass_ = VK_NULL_HANDLE;
};

}
