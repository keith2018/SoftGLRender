/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "RendererVulkan.h"
#include "Base/Logger.h"
#include "TextureVulkan.h"
#include "VertexVulkan.h"
#include "UniformVulkan.h"
#include "VulkanUtils.h"

namespace SoftGL {

bool RendererVulkan::create() {
  bool success = false;
#ifdef DEBUG
  success = vkCtx_.create(true);
#endif
  success = vkCtx_.create(false);
  if (success) {
    device_ = vkCtx_.device();
    createCommandBuffer();
  }
  return success;
}

void RendererVulkan::destroy() {
  vkFreeCommandBuffers(device_, vkCtx_.getCommandPool(), 1, &copyCmd_);
  vkFreeCommandBuffers(device_, vkCtx_.getCommandPool(), 1, &drawCmd_);
  vkCtx_.destroy();
}

// framebuffer
std::shared_ptr<FrameBuffer> RendererVulkan::createFrameBuffer() {
  return std::make_shared<FrameBufferVulkan>(vkCtx_);
}

// texture
std::shared_ptr<Texture> RendererVulkan::createTexture(const TextureDesc &desc) {
  switch (desc.type) {
    case TextureType_2D:    return std::make_shared<Texture2DVulkan>(vkCtx_, desc);
    case TextureType_CUBE:  return std::make_shared<TextureCubeVulkan>(vkCtx_, desc);
  }
  return nullptr;
}

// vertex
std::shared_ptr<VertexArrayObject> RendererVulkan::createVertexArrayObject(const VertexArray &vertexArray) {
  return std::make_shared<VertexArrayObjectVulkan>(vkCtx_, vertexArray);
}

// shader program
std::shared_ptr<ShaderProgram> RendererVulkan::createShaderProgram() {
  return std::make_shared<ShaderProgramVulkan>(vkCtx_);
}

// pipeline states
std::shared_ptr<PipelineStates> RendererVulkan::createPipelineStates(const RenderStates &renderStates) {
  return std::make_shared<PipelineStatesVulkan>(vkCtx_, renderStates);
}

// uniform
std::shared_ptr<UniformBlock> RendererVulkan::createUniformBlock(const std::string &name, int size) {
  return std::make_shared<UniformBlockVulkan>(vkCtx_, name, size);
}

std::shared_ptr<UniformSampler> RendererVulkan::createUniformSampler(const std::string &name, const TextureDesc &desc) {
  return std::make_shared<UniformSamplerVulkan>(vkCtx_, name, desc.type, desc.format);
}

// pipeline
void RendererVulkan::setFrameBuffer(std::shared_ptr<FrameBuffer> &frameBuffer) {
  if (!frameBuffer) {
    return;
  }

  fbo_ = dynamic_cast<FrameBufferVulkan *>(frameBuffer.get());
  fbo_->create();
  renderPass_ = fbo_->getRenderPass();
}

void RendererVulkan::setViewPort(int x, int y, int width, int height) {
  viewport_.x = (float) x;
  viewport_.y = (float) y;
  viewport_.width = (float) width;
  viewport_.height = (float) height;
  viewport_.minDepth = 0.0f;
  viewport_.maxDepth = 1.0f;
}

void RendererVulkan::clear(const ClearStates &state) {
  clearValues_.clear();
  if (state.colorFlag) {
    VkClearValue clearColor;
    clearColor.color = {state.clearColor.r, state.clearColor.g, state.clearColor.b, state.clearColor.a};
    clearValues_.push_back(clearColor);
  }

  if (state.depthFlag) {
    VkClearValue clearDepth;
    clearDepth.depthStencil.depth = viewport_.maxDepth;
    clearValues_.push_back(clearDepth);
  }
}

void RendererVulkan::setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) {
  if (!vao) {
    return;
  }

  vao_ = dynamic_cast<VertexArrayObjectVulkan *>(vao.get());
}

void RendererVulkan::setShaderProgram(std::shared_ptr<ShaderProgram> &program) {
  shaderProgram_ = dynamic_cast<ShaderProgramVulkan *>(program.get());
}

void RendererVulkan::setShaderResources(std::shared_ptr<ShaderResources> &resources) {
  if (!resources) {
    return;
  }

  if (shaderProgram_) {
    bool needBind = shaderProgram_->bindUniformsBegin(resources->blocks.size() + resources->samplers.size());
    if (needBind) {
      shaderProgram_->bindResources(*resources);
      shaderProgram_->bindUniformsEnd();
    }
  }
}

void RendererVulkan::setPipelineStates(std::shared_ptr<PipelineStates> &states) {
  pipelineStates_ = dynamic_cast<PipelineStatesVulkan *>(states.get());
  pipelineStates_->create(vao_->getVertexInputInfo(), shaderProgram_, renderPass_);
}

void RendererVulkan::draw() {
  if (pipelineStates_->getGraphicsPipeline() == VK_NULL_HANDLE) {
    return;
  }

  recordDraw(drawCmd_);
  submitWork(drawCmd_, vkCtx_.getGraphicsQueue());

  recordCopy(copyCmd_);
  submitWork(copyCmd_, vkCtx_.getGraphicsQueue());
}

bool RendererVulkan::createCommandBuffer() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = vkCtx_.getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VK_CHECK(vkAllocateCommandBuffers(device_, &allocInfo, &drawCmd_));
  VK_CHECK(vkAllocateCommandBuffers(device_, &allocInfo, &copyCmd_));
  return true;
}

void RendererVulkan::recordDraw(VkCommandBuffer commandBuffer) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  // render pass
  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass_;
  renderPassInfo.framebuffer = fbo_->getVKFramebuffer();
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = {fbo_->width(), fbo_->height()};

  renderPassInfo.clearValueCount = clearValues_.size();
  renderPassInfo.pClearValues = clearValues_.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  // pipe line
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineStates_->getGraphicsPipeline());

  // view port
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport_);

  // vertex buffer
  VkBuffer vertexBuffers[] = {vao_->getVertexBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  // index buffer
  vkCmdBindIndexBuffer(commandBuffer, vao_->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

  // descriptor sets
  auto &descriptorSets = shaderProgram_->getVkDescriptorSet();
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineStates_->getGraphicsPipelineLayout(),
                          0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

  // draw
  vkCmdDrawIndexed(commandBuffer, vao_->getIndicesCnt(), 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);
  VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void RendererVulkan::recordCopy(VkCommandBuffer commandBuffer) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  // Transition destination image to transfer destination layout
  VkImageMemoryBarrier imageMemoryBarrier{};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.srcAccessMask = 0;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imageMemoryBarrier.image = fbo_->getHostImage();
  imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &imageMemoryBarrier);

  // colorAttachment.image is already in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and does not need to be transitioned

  VkImageCopy imageCopyRegion{};
  imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.srcSubresource.layerCount = 1;
  imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.dstSubresource.layerCount = 1;
  imageCopyRegion.extent.width = fbo_->width();
  imageCopyRegion.extent.height = fbo_->height();
  imageCopyRegion.extent.depth = 1;

  vkCmdCopyImage(commandBuffer,
                 fbo_->getColorAttachmentImage(),
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 fbo_->getHostImage(),
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1,
                 &imageCopyRegion);

  // Transition destination image to general layout, which is the required layout for mapping the image memory later on
  imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &imageMemoryBarrier);

  VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void RendererVulkan::submitWork(VkCommandBuffer cmdBuffer, VkQueue queue) {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = 0;

  VkFence fence;
  VK_CHECK(vkCreateFence(device_, &fenceInfo, nullptr, &fence));

  VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));

  VK_CHECK(vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX));
  vkDestroyFence(device_, fence, nullptr);
}

}
