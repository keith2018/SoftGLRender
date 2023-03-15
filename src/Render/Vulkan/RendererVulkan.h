/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "Base/Platform.h"
#include "Render/Renderer.h"

#ifdef PLATFORM_OSX
#include "MoltenVK/mvk_vulkan.h"
#else
#include "vukan/vukan.h"
#endif

namespace SoftGL {

struct QueueFamilyIndices {
  int32_t graphicsFamily = -1;

  bool isComplete() const {
    return graphicsFamily >= 0;
  }
};

struct FrameBufferAttachment {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
};

class RendererVulkan : public Renderer {
 public:
  RendererVulkan();
  ~RendererVulkan();

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
  std::shared_ptr<UniformSampler> createUniformSampler(const std::string &name, TextureType type,
                                                       TextureFormat format) override;

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
  bool createFrameBuffers();
  bool createCommandPool();
  bool createCommandBuffer();

  void recordCommandBuffer(VkCommandBuffer commandBuffer);
  void submitWork(VkCommandBuffer cmdBuffer, VkQueue queue);

  bool checkValidationLayerSupport();
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool createShaderModule(VkShaderModule &shaderModule, const std::string &code);
  uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

 private:
  bool enableValidationLayers_ = false;
  VkDebugUtilsMessengerEXT debugMessenger_;

  VkInstance instance_;
  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
  VkDevice device_;
  VkQueue graphicsQueue_;

  uint32_t width_ = 1024, height_ = 1024;
  VkFramebuffer framebuffer_;
  FrameBufferAttachment colorAttachment_;

  VkRenderPass renderPass_;
  VkPipelineLayout pipelineLayout_;
  VkPipeline graphicsPipeline_;

  VkCommandPool commandPool_;
  VkCommandBuffer commandBuffer_;
};

}
