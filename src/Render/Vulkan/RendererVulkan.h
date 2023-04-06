/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/Platform.h"
#include "Render/Renderer.h"
#include "FramebufferVulkan.h"
#include "VertexVulkan.h"
#include "ShaderProgramVulkan.h"
#include "PipelineStatesVulkan.h"
#include "VulkanUtils.h"

namespace SoftGL {

class RendererVulkan : public Renderer {
 public:
  bool create() override;
  void destroy() override;

  // framebuffer
  std::shared_ptr<FrameBuffer> createFrameBuffer(bool offscreen) override;

  // texture
  std::shared_ptr<Texture> createTexture(const TextureDesc &desc) override;

  // vertex
  std::shared_ptr<VertexArrayObject> createVertexArrayObject(const VertexArray &vertexArray) override;

  // shader program
  std::shared_ptr<ShaderProgram> createShaderProgram() override;

  // pipeline states
  std::shared_ptr<PipelineStates> createPipelineStates(const RenderStates &renderStates) override;

  // uniform
  std::shared_ptr<UniformBlock> createUniformBlock(const std::string &name, int size) override;
  std::shared_ptr<UniformSampler> createUniformSampler(const std::string &name, const TextureDesc &desc) override;

  // pipeline
  void beginRenderPass(std::shared_ptr<FrameBuffer> &frameBuffer, const ClearStates &states) override;
  void setViewPort(int x, int y, int width, int height) override;
  void setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) override;
  void setShaderProgram(std::shared_ptr<ShaderProgram> &program) override;
  void setShaderResources(std::shared_ptr<ShaderResources> &resources) override;
  void setPipelineStates(std::shared_ptr<PipelineStates> &states) override;
  void draw() override;
  void endRenderPass() override;

 private:
  FrameBufferVulkan *fbo_ = nullptr;
  VertexArrayObjectVulkan *vao_ = nullptr;
  ShaderProgramVulkan *shaderProgram_ = nullptr;
  PipelineStatesVulkan *pipelineStates_ = nullptr;

  VkViewport viewport_{};
  std::vector<VkClearValue> clearValues_;

  VKContext vkCtx_;
  VkDevice device_ = VK_NULL_HANDLE;

  CommandBuffer *commandBuffer_ = nullptr;
  VkCommandBuffer drawCmd_ = VK_NULL_HANDLE;
  VkSemaphore lastPassSemaphore_ = VK_NULL_HANDLE;
};

}
