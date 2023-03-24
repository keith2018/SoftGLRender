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
    prepare();
  }
  return success;
}

void RendererVulkan::destroy() {
  vkDestroyFence(device_, drawFence_, nullptr);
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
    shaderProgram_->beginBindUniforms(resources->blocks.size() + resources->samplers.size());
    shaderProgram_->bindResources(*resources);
    shaderProgram_->endBindUniforms();
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

  vkWaitForFences(device_, 1, &drawFence_, VK_TRUE, UINT64_MAX);
  vkResetFences(device_, 1, &drawFence_);
  vkResetCommandBuffer(drawCmd_, 0);
  recordDraw(drawCmd_);
  vkCtx_.submitWork(drawCmd_, drawFence_);

  // reset clear values
  clearValues_.clear();
}

void RendererVulkan::prepare() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = vkCtx_.getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;
  VK_CHECK(vkAllocateCommandBuffers(device_, &allocInfo, &drawCmd_));

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VK_CHECK(vkCreateFence(device_, &fenceInfo, nullptr, &drawFence_));
}

void RendererVulkan::recordDraw(VkCommandBuffer commandBuffer) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  // render pass
  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass_;
  renderPassInfo.framebuffer = fbo_->getVkFramebuffer();
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

}
