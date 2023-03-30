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
    vkDestroyRenderPass(device_, renderPass_, nullptr);
  }

  int getId() const override {
    return uuid_.get();
  }

  bool isValid() override {
    return colorReady || depthReady;
  }

  void create(const ClearStates &states) {
    clearStates_ = states;
    createRenderPass();
    createFramebuffer();
  }

  inline ClearStates &getClearStates() {
    return clearStates_;
  }

  inline VkRenderPass &getRenderPass() {
    return renderPass_;
  }

  inline VkFramebuffer &getVkFramebuffer() {
    return framebuffer_;
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

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::vector<VkAttachmentDescription> attachments;
    if (colorReady) {
      auto *colorDesc = getColorAttachmentDesc();

      VkAttachmentDescription colorAttachment{};
      colorAttachment.format = VK::cvtImageFormat(colorDesc->format, colorDesc->usage);
      colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp = clearStates_.colorFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      colorAttachmentRef.attachment = attachments.size();
      attachments.push_back(colorAttachment);
    }
    if (depthReady) {
      auto *depthDesc = getDepthAttachmentDesc();

      VkAttachmentDescription depthAttachment{};
      depthAttachment.format = VK::cvtImageFormat(depthDesc->format, depthDesc->usage);
      depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      depthAttachment.loadOp = clearStates_.depthFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
      depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      depthAttachmentRef.attachment = attachments.size();
      attachments.push_back(depthAttachment);
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;

    if (colorReady) {
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colorAttachmentRef;
    }
    if (depthReady) {
      subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }

    std::vector<VkSubpassDependency> dependencies;

    if (!colorReady) {
      // shadow depth pass
      dependencies.resize(2);
      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      dependencies[1].srcSubpass = 0;
      dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    } else {
      // normal render pass
      dependencies.resize(1);
      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[0].srcAccessMask = 0;
      dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      if (depthReady) {
        dependencies[0].srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      }
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = dependencies.size();
    renderPassInfo.pDependencies = dependencies.data();

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
      attachments[attachCnt] = texColor->createImageView();
      width_ = texColor->width;
      height_ = texColor->height;
      attachCnt++;
    }
    if (depthReady) {
      auto *texDepth = dynamic_cast<Texture2DVulkan *>(depthAttachment.get());
      attachments[attachCnt] = texDepth->createImageView();
      width_ = texDepth->width;
      height_ = texDepth->height;
      attachCnt++;
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

 private:
  UUID<FrameBufferVulkan> uuid_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
  VkRenderPass renderPass_ = VK_NULL_HANDLE;

  ClearStates clearStates_{};
};

}
