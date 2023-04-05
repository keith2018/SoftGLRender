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
  }
  return success;
}

void RendererVulkan::destroy() {
  vkCtx_.destroy();
}

// framebuffer
std::shared_ptr<FrameBuffer> RendererVulkan::createFrameBuffer(bool offscreen) {
  return std::make_shared<FrameBufferVulkan>(vkCtx_, offscreen);
}

// texture
std::shared_ptr<Texture> RendererVulkan::createTexture(const TextureDesc &desc) {
  switch (desc.type) {
    case TextureType_2D:
    case TextureType_CUBE:
      return std::make_shared<TextureVulkan>(vkCtx_, desc);
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
void RendererVulkan::beginRenderPass(std::shared_ptr<FrameBuffer> &frameBuffer, const ClearStates &states) {
  if (!frameBuffer) {
    return;
  }

  fbo_ = dynamic_cast<FrameBufferVulkan *>(frameBuffer.get());
  if (!fbo_->create(states)) {
    LOGE("VulkanRenderer init framebuffer failed");
  }

  // clear operation controlled by render pass load op
  clearValues_.clear();
  if (fbo_->isColorReady()) {
    VkClearValue colorClear;
    colorClear.color = {states.clearColor.r, states.clearColor.g, states.clearColor.b, states.clearColor.a};
    clearValues_.push_back(colorClear);
  }
  if (fbo_->isDepthReady()) {
    VkClearValue depthClear;
    depthClear.depthStencil = {states.clearDepth, 0};
    clearValues_.push_back(depthClear);
  }

  commandBuffer_ = &vkCtx_.beginCommands();
  drawCmd_ = commandBuffer_->cmdBuffer;

  // render pass
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = fbo_->getRenderPass();
  renderPassInfo.framebuffer = fbo_->getVkFramebuffer();
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = {fbo_->width(), fbo_->height()};
  renderPassInfo.clearValueCount = clearValues_.size();
  renderPassInfo.pClearValues = clearValues_.data();

  vkCmdBeginRenderPass(drawCmd_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RendererVulkan::setViewPort(int x, int y, int width, int height) {
  viewport_.x = (float) x;
  viewport_.y = (float) y;
  viewport_.width = (float) width;
  viewport_.height = (float) height;
  viewport_.minDepth = 0.f;
  viewport_.maxDepth = 1.f;
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
    shaderProgram_->beginBindUniforms();
    shaderProgram_->bindResources(*resources);
    shaderProgram_->endBindUniforms();
  }
}

void RendererVulkan::setPipelineStates(std::shared_ptr<PipelineStates> &states) {
  pipelineStates_ = dynamic_cast<PipelineStatesVulkan *>(states.get());
  pipelineStates_->create(vao_->getVertexInputInfo(), shaderProgram_, fbo_->getRenderPass(), fbo_->getSampleCount());
}

void RendererVulkan::draw() {
  // pipeline
  vkCmdBindPipeline(drawCmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineStates_->getGraphicsPipeline());

  // viewport
  vkCmdSetViewport(drawCmd_, 0, 1, &viewport_);

  // vertex buffer
  VkBuffer vertexBuffers[] = {vao_->getVertexBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(drawCmd_, 0, 1, vertexBuffers, offsets);

  // index buffer
  vkCmdBindIndexBuffer(drawCmd_, vao_->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

  // descriptor sets
  auto &descriptorSets = shaderProgram_->getVkDescriptorSet();
  vkCmdBindDescriptorSets(drawCmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineStates_->getGraphicsPipelineLayout(),
                          0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

  // draw
  vkCmdDrawIndexed(drawCmd_, vao_->getIndicesCnt(), 1, 0, 0, 0);
}

void RendererVulkan::endRenderPass() {
  vkCmdEndRenderPass(drawCmd_);

  VkSemaphore currSemaphore = commandBuffer_->semaphore;
  vkCtx_.endCommands(*commandBuffer_, lastPassSemaphore_);
  lastPassSemaphore_ = currSemaphore;
}

}
