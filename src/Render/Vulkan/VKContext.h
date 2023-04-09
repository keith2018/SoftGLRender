/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "VulkanLoader.h"

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
  VkDeviceSize allocationSize = 0;

  void destroy(VkDevice device) {
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, memory, nullptr);

    image = VK_NULL_HANDLE;
    memory = VK_NULL_HANDLE;
  }
};

struct AllocatedBuffer {
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkDeviceSize allocationSize = 0;

  void destroy(VkDevice device) {
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);

    buffer = VK_NULL_HANDLE;
    memory = VK_NULL_HANDLE;
    allocationSize = 0;
  }
};

struct UniformBuffer {
  AllocatedBuffer buffer{};
  void *mapPtr = nullptr;
  bool inUse = false;
};

struct DescriptorSet {
  VkDescriptorSet set = VK_NULL_HANDLE;
  bool inUse = false;
};

struct CommandBuffer {
  VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
  VkSemaphore semaphore = VK_NULL_HANDLE;
  VkFence fence = VK_NULL_HANDLE;
  std::vector<UniformBuffer *> uniformBuffers;
  std::vector<DescriptorSet *> descriptorSets;
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

  UniformBuffer *getNewUniformBuffer(VkDeviceSize size);

  CommandBuffer *beginCommands();
  void endCommands(CommandBuffer *commandBuffer,
                   const std::vector<VkSemaphore> &waitSemaphores = {},
                   const std::vector<VkSemaphore> &signalSemaphores = {});
  void waitCommands(CommandBuffer *commandBuffer);

  void createBuffer(AllocatedBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
  void createStagingBuffer(AllocatedBuffer &buffer, VkDeviceSize size);
  bool createImageMemory(AllocatedImage &image, uint32_t properties, void *pNext = nullptr);

  bool linearBlitAvailable(VkFormat imageFormat);

 private:
  bool createInstance();
  bool setupDebugMessenger();
  bool pickPhysicalDevice();
  bool createLogicalDevice();
  bool createCommandPool();

  CommandBuffer *getNewCommandBuffer();
  void purgeCommandBuffers();
  void allocateCommandBuffer(VkCommandBuffer &cmdBuffer);

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

  // command buffer pool
  std::vector<CommandBuffer> commandBuffers_;

  // uniform buffer pool
  std::unordered_map<VkDeviceSize, std::vector<UniformBuffer>> uniformBufferPool_;

  size_t maxCommandBufferPoolSize_ = 0;
  size_t maxUniformBufferPoolSize_ = 0;

  VkPhysicalDeviceProperties deviceProperties_{};
  VkPhysicalDeviceMemoryProperties deviceMemoryProperties_{};
};

}
