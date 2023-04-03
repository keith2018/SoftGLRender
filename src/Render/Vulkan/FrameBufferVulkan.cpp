/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "FramebufferVulkan.h"


namespace SoftGL {

void FrameBufferVulkan::createVkRenderPass() {
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
  if (colorReady_) {
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

  if (depthReady_) {
    auto *depthTex = getAttachmentDepth();

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK::cvtImageFormat(depthTex->format, depthTex->usage);
    depthAttachment.samples = getAttachmentDepth()->getSampleCount();
    depthAttachment.loadOp = clearStates_.depthFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = colorReady_ ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    depthAttachmentRef.attachment = attachments.size();
    attachments.push_back(depthAttachment);
  }

  if (colorReady_) {
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
  subpass.colorAttachmentCount = colorReady_ ? 1 : 0;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
  subpass.pResolveAttachments = &resolveAttachmentRef;

  std::vector<VkSubpassDependency> dependencies;

  if (!colorReady_) {
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

    if (depthReady_) {
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

bool FrameBufferVulkan::createVkFramebuffer() {
  std::vector<VkImageView> attachments;
  if (colorReady_) {
    auto *texColor = getAttachmentColor();
    attachments.push_back(texColor->createAttachmentView(colorAttachment_.layer, colorAttachment_.level));
  }
  if (depthReady_) {
    auto *texDepth = getAttachmentDepth();
    attachments.push_back(texDepth->createAttachmentView(depthAttachment_.layer, depthAttachment_.level));
  }
  // color resolve
  if (colorReady_ && isMultiSample()) {
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

}
