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

  void setColorAttachment(std::shared_ptr<Texture> &color, int level) override {
    FrameBuffer::setColorAttachment(color, level);
    width_ = color->width;
    height_ = color->height;
    dirty_ = true;
  }

  void setColorAttachment(std::shared_ptr<Texture> &color, CubeMapFace face, int level) override {
    FrameBuffer::setColorAttachment(color, face, level);
    width_ = color->width;
    height_ = color->height;
    dirty_ = true;
  }

  void setDepthAttachment(std::shared_ptr<Texture> &depth) override {
    FrameBuffer::setDepthAttachment(depth);
    width_ = depth->width;
    height_ = depth->height;
    dirty_ = true;
  }

  bool create(const ClearStates &states) {
    dirty_ = false;
    clearStates_ = states;
    createRenderPass();
    return createFramebuffer();
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

  inline VkSampleCountFlagBits getSampleCount() {
    if (colorReady) {
      return getAttachmentColor()->getSampleCount();
    }

    if (depthReady) {
      return getAttachmentDepth()->getSampleCount();
    }

    return VK_SAMPLE_COUNT_1_BIT;
  }

  inline uint32_t width() const {
    return width_;
  }

  inline uint32_t height() const {
    return height_;
  }

 private:
  void createRenderPass() {
    if (!dirty_ && renderPass_ != VK_NULL_HANDLE) {
      return;
    }

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = VK_ATTACHMENT_UNUSED;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = VK_ATTACHMENT_UNUSED;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveAttachmentRef{};
    resolveAttachmentRef.attachment = VK_ATTACHMENT_UNUSED;
    resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::vector<VkAttachmentDescription> attachments;
    if (colorReady) {
      auto *colorTex = getAttachmentColor();

      VkAttachmentDescription colorAttachment{};
      colorAttachment.format = VK::cvtImageFormat(colorTex->format, colorTex->usage);
      colorAttachment.samples = getAttachmentColor()->getSampleCount();
      colorAttachment.loadOp = clearStates_.colorFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : (colorTex->multiSample ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
                                                                                                             : VK_ATTACHMENT_LOAD_OP_LOAD);
      colorAttachment.storeOp = colorTex->multiSample > VK_SAMPLE_COUNT_1_BIT ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      colorAttachmentRef.attachment = attachments.size();
      attachments.push_back(colorAttachment);
    }

    if (depthReady) {
      auto *depthTex = getAttachmentDepth();

      VkAttachmentDescription depthAttachment{};
      depthAttachment.format = VK::cvtImageFormat(depthTex->format, depthTex->usage);
      depthAttachment.samples = getAttachmentDepth()->getSampleCount();
      depthAttachment.loadOp = clearStates_.depthFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
      depthAttachment.storeOp = colorReady ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
      depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      depthAttachmentRef.attachment = attachments.size();
      attachments.push_back(depthAttachment);
    }

    if (colorReady) {
      auto *colorTex = getAttachmentColor();
      // color resolve
      if (colorTex->multiSample) {
        VkAttachmentDescription colorResolveAttachment{};
        colorResolveAttachment.format = VK::cvtImageFormat(colorTex->format, colorTex->usage);
        colorResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        resolveAttachmentRef.attachment = attachments.size();
        attachments.push_back(colorResolveAttachment);
      }
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorReady ? 1 : 0;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &resolveAttachmentRef;

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

  bool createFramebuffer() {
    if (!dirty_ && framebuffer_ != VK_NULL_HANDLE) {
      return true;
    }

    if (!isValid()) {
      return false;
    }

    std::vector<VkImageView> attachments;
    if (colorReady) {
      auto *texColor = getAttachmentColor();
      attachments.push_back(texColor->getImageView());
    }
    if (depthReady) {
      auto *texDepth = getAttachmentDepth();
      attachments.push_back(texDepth->getImageView());
    }
    // color resolve
    if (colorReady && isMultiSample()) {
      auto *texColor = getAttachmentColor();
      attachments.push_back(texColor->getImageViewResolve());
    }

    VkFramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = renderPass_;
    framebufferCreateInfo.attachmentCount = attachments.size();
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.width = width_;
    framebufferCreateInfo.height = height_;
    framebufferCreateInfo.layers = 1;
    VK_CHECK(vkCreateFramebuffer(device_, &framebufferCreateInfo, nullptr, &framebuffer_));

    return true;
  }

  inline Texture2DVulkan *getAttachmentColor() {
    switch (colorTexType) {
      case TextureType_2D: {
        return dynamic_cast<Texture2DVulkan *>(colorAttachment2d.tex.get());
      }
      case TextureType_CUBE: {
        return dynamic_cast<Texture2DVulkan *>(colorAttachmentCube.tex.get());
      }
      default:
        break;
    }

    return nullptr;
  }

  inline Texture2DVulkan *getAttachmentDepth() {
    return dynamic_cast<Texture2DVulkan *>(depthAttachment.get());
  }

 private:
  UUID<FrameBufferVulkan> uuid_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  bool dirty_ = true;

  VKContext &vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
  VkRenderPass renderPass_ = VK_NULL_HANDLE;

  ClearStates clearStates_{};
};

}
