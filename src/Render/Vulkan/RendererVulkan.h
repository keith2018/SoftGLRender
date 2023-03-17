/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/Platform.h"
#include "Render/Renderer.h"
#include "FramebufferVulkan.h"
#include "ShaderProgramVulkan.h"
#include "VulkanUtils.h"

namespace SoftGL {

struct QueueFamilyIndices {
  int32_t graphicsFamily = -1;

  bool isComplete() const {
    return graphicsFamily >= 0;
  }
};

class RendererVulkan : public Renderer {
 public:
  bool create() override;
  void destroy() override;

  // framebuffer
  std::shared_ptr<FrameBuffer> createFrameBuffer() override;

  // texture
  std::shared_ptr<Texture> createTexture(const TextureDesc &desc) override;

  // vertex
  std::shared_ptr<VertexArrayObject> createVertexArrayObject(const VertexArray &vertexArray) override;

  // shader program
  std::shared_ptr<ShaderProgram> createShaderProgram() override;

  // uniform
  std::shared_ptr<UniformBlock> createUniformBlock(const std::string &name, int size) override;
  std::shared_ptr<UniformSampler> createUniformSampler(const std::string &name, const TextureDesc &desc) override;

  // pipeline
  void setFrameBuffer(std::shared_ptr<FrameBuffer> &frameBuffer) override;
  void setViewPort(int x, int y, int width, int height) override;
  void clear(const ClearState &state) override;
  void setRenderState(const RenderState &state) override;
  void setVertexArrayObject(std::shared_ptr<VertexArrayObject> &vao) override;
  void setShaderProgram(std::shared_ptr<ShaderProgram> &program) override;
  void setShaderUniforms(std::shared_ptr<ShaderUniforms> &uniforms) override;
  void draw(PrimitiveType type) override;

 private:
  bool initVulkan();
  void cleanupVulkan();

  bool createInstance();
  bool setupDebugMessenger();
  bool pickPhysicalDevice();
  bool createLogicalDevice();
  bool createRenderPass();
  bool createGraphicsPipeline();
  bool createCommandPool();
  bool createCommandBuffer();

  void recordDraw(VkCommandBuffer commandBuffer);
  void recordCopy(VkCommandBuffer commandBuffer);
  void submitWork(VkCommandBuffer cmdBuffer, VkQueue queue);

  bool checkValidationLayerSupport();
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

 private:
  FrameBufferVulkan *fbo_ = nullptr;
  ShaderProgramVulkan *shaderProgram_ = nullptr;

  VkClearValue clearValue_;
  VkViewport viewport_;

  bool enableValidationLayers_ = false;
  VkDebugUtilsMessengerEXT debugMessenger_;
  VKContext vkCtx_;

  VkRenderPass renderPass_;
  VkPipelineLayout pipelineLayout_;
  VkPipeline graphicsPipeline_;

  VkCommandPool commandPool_;

  VkCommandBuffer drawCmd_;
  VkCommandBuffer copyCmd_;
};

}
