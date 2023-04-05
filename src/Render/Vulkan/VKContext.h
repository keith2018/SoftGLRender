/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include <vector>
#include "VulkanInc.h"

namespace SoftGL {

struct QueueFamilyIndices {
  int32_t graphicsFamily = -1;

  bool isComplete() const {
    return graphicsFamily >= 0;
  }
};

struct AllocatedImage {
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct AllocatedBuffer {
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkDeviceSize size = 0;
};

struct CommandBuffer {
  VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
  VkSemaphore semaphore = VK_NULL_HANDLE;
  VkFence fence = VK_NULL_HANDLE;
  bool inUse = false;
};

class VKContext {
 public:
  bool create(bool debugOutput = false);
  void destroy();

  inline const VkInstance &instance() const {
    return instance_;
  }

  inline const VkPhysicalDevice &physicalDevice() const {
    return physicalDevice_;
  }

  inline const VkDevice &device() const {
    return device_;
  }

  inline VkQueue &getGraphicsQueue() {
    return graphicsQueue_;
  }

  inline VkCommandPool &getCommandPool() {
    return commandPool_;
  }

  inline VkPhysicalDeviceProperties &getPhysicalDeviceProperties() {
    return deviceProperties_;
  }

  uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

  CommandBuffer &getCommandBuffer();
  void purgeCommandBuffers();

  CommandBuffer &beginCommands();
  void endCommands(CommandBuffer &commandBuffer, VkSemaphore semaphore = VK_NULL_HANDLE, bool waitOnHost = false);

  void createBuffer(AllocatedBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
  void createStagingBuffer(AllocatedBuffer &buffer, VkDeviceSize size);
  bool createImageMemory(AllocatedImage &image, uint32_t properties);

  bool linearBlitAvailable(VkFormat imageFormat);

  static void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
                                    VkImageSubresourceRange subresourceRange,
                                    VkAccessFlags srcMask,
                                    VkAccessFlags dstMask,
                                    VkImageLayout oldLayout,
                                    VkImageLayout newLayout,
                                    VkPipelineStageFlags srcStage,
                                    VkPipelineStageFlags dstStage);

 private:
  bool createInstance();
  bool setupDebugMessenger();
  bool pickPhysicalDevice();
  bool createLogicalDevice();
  bool createCommandPool();

  void AllocateCommandBuffer(VkCommandBuffer &cmdBuffer);

  static bool checkValidationLayerSupport();
  static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
  static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);

 private:
  bool debugOutput_ = false;
  VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  QueueFamilyIndices queueIndices_;
  VkQueue graphicsQueue_ = VK_NULL_HANDLE;
  VkCommandPool commandPool_ = VK_NULL_HANDLE;

  std::vector<CommandBuffer> commandBuffers_;

  VkPhysicalDeviceProperties deviceProperties_{};
  VkPhysicalDeviceMemoryProperties deviceMemoryProperties_{};
};

}
